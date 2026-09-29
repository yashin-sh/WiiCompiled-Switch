"""Scan a user-owned RMCP01 image for direct GX texture-call callsites.

This tool never embeds game data. It scans a local MEM1 dump or a DTK-merged
ELF and reports direct PowerPC branch-and-link instructions targeting selected
GX functions. Public doldecomp/mkw metadata and the optional MKW-SP Ghidra XML
can then attribute each callsite without committing Nintendo content.
"""

from __future__ import annotations

import argparse
import bisect
import importlib.util
import json
import re
import struct
import sys
import tempfile
from collections.abc import Iterable
from dataclasses import asdict, dataclass
from pathlib import Path
from types import ModuleType

REPO_ROOT = Path(__file__).resolve().parents[1]
ATTRIBUTE_HELPER = REPO_ROOT / "scripts" / "attribute-rmcp01-address.py"

DEFAULT_TARGETS = {
    0x80170A4C: "GXInitTexObjLOD",
    0x80170B50: "GXInitTexObjWrapMode",
    0x801707F8: "GXInitTexObj",
    0x80170F2C: "GXLoadTexObj",
}

FUNCTION_RE = re.compile(
    rb'<FUNCTION ENTRY_POINT="([0-9A-Fa-f]+)" NAME="([^"]+)"[^>]*>\s*'
    rb'<ADDRESS_RANGE START="([0-9A-Fa-f]+)" END="([0-9A-Fa-f]+)"',
    re.DOTALL,
)


@dataclass(frozen=True)
class ExecutableRange:
    address: int
    data: bytes
    name: str


@dataclass(frozen=True)
class GhidraFunction:
    start: int
    end: int
    name: str


@dataclass
class Callsite:
    callsite: str
    target: str
    target_symbol: str
    image_range: str
    ghidra_function: str | None
    ghidra_function_range: str | None
    module: str | None
    object_path: str | None
    public_symbol: str | None
    public_source: str | None
    confidence: str


def sign_extend(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def ppc_direct_branch_target(address: int, instruction: int) -> int | None:
    """Return a direct PPC b/bl target, or None for non-direct branches.

    PowerPC opcode 18 encodes a 26-bit signed displacement including the two
    low zero bits. AA selects absolute vs PC-relative addressing. LK is not
    checked here so the helper can be self-tested for both b and bl.
    """
    if instruction >> 26 != 18:
        return None
    displacement = sign_extend(instruction & 0x03FFFFFC, 26)
    if instruction & 0x2:  # AA
        return displacement & 0xFFFFFFFF
    return (address + displacement) & 0xFFFFFFFF


def encode_bl(address: int, target: int, absolute: bool = False) -> int:
    if absolute:
        displacement = target
        aa = 0x2
    else:
        displacement = (target - address) & 0xFFFFFFFF
        if displacement & 0x80000000:
            displacement -= 1 << 32
        aa = 0
    if displacement % 4 != 0 or not -(1 << 25) <= displacement < (1 << 25):
        raise ValueError("branch target is out of PPC direct-branch range")
    return 0x48000001 | aa | (displacement & 0x03FFFFFC)


def scan_ranges(
    ranges: Iterable[ExecutableRange], targets: dict[int, str]
) -> list[tuple[int, int, str]]:
    found: list[tuple[int, int, str]] = []
    for region in ranges:
        limit = len(region.data) - (len(region.data) % 4)
        for offset in range(0, limit, 4):
            instruction = struct.unpack_from(">I", region.data, offset)[0]
            if instruction & 1 == 0:  # LK=0 => branch, not call
                continue
            address = (region.address + offset) & 0xFFFFFFFF
            target = ppc_direct_branch_target(address, instruction)
            if target is not None and target in targets:
                found.append((address, target, region.name))
    return found


def raw_ranges(path: Path, base: int) -> list[ExecutableRange]:
    return [ExecutableRange(base, path.read_bytes(), path.name)]


def elf_ranges(path: Path) -> list[ExecutableRange]:
    data = path.read_bytes()
    if len(data) < 0x34 or data[:4] != b"\x7fELF":
        raise ValueError(f"{path} is not an ELF file")

    elf_class = data[4]
    data_encoding = data[5]
    if data_encoding == 1:
        endian = "<"
    elif data_encoding == 2:
        endian = ">"
    else:
        raise ValueError("unsupported ELF data encoding")

    if elf_class == 1:
        header_fmt = endian + "16sHHIIIIIHHHHHH"
        sh_fmt = endian + "IIIIIIIIII"
        header = struct.unpack_from(header_fmt, data, 0)
        shoff, shentsize, shnum, shstrndx = (
            header[6],
            header[11],
            header[12],
            header[13],
        )
        addr_index, offset_index, size_index, flags_index = 3, 4, 5, 2
    elif elf_class == 2:
        header_fmt = endian + "16sHHIQQQIHHHHHH"
        sh_fmt = endian + "IIQQQQIIQQ"
        header = struct.unpack_from(header_fmt, data, 0)
        shoff, shentsize, shnum, shstrndx = (
            header[6],
            header[11],
            header[12],
            header[13],
        )
        addr_index, offset_index, size_index, flags_index = 3, 4, 5, 2
    else:
        raise ValueError("unsupported ELF class")

    expected_sh = struct.calcsize(sh_fmt)
    if shentsize < expected_sh or shnum == 0:
        raise ValueError("ELF has no usable section headers")

    sections = []
    for index in range(shnum):
        off = shoff + index * shentsize
        if off + expected_sh > len(data):
            raise ValueError("ELF section table is truncated")
        sections.append(struct.unpack_from(sh_fmt, data, off))

    if shstrndx >= len(sections):
        raise ValueError("invalid ELF shstrndx")
    shstr = sections[shstrndx]
    names_offset = shstr[offset_index]
    names_size = shstr[size_index]
    names = data[names_offset : names_offset + names_size]

    def name_at(offset: int) -> str:
        if offset >= len(names):
            return "<unnamed>"
        end = names.find(b"\0", offset)
        if end < 0:
            end = len(names)
        return names[offset:end].decode("utf-8", errors="replace") or "<unnamed>"

    result: list[ExecutableRange] = []
    for section in sections:
        sh_name = section[0]
        sh_type = section[1]
        sh_flags = section[flags_index]
        sh_addr = section[addr_index]
        sh_offset = section[offset_index]
        sh_size = section[size_index]
        if sh_type != 1 or sh_flags & 0x4 == 0 or sh_size == 0:
            continue
        if sh_offset + sh_size > len(data):
            raise ValueError(f"ELF executable section {name_at(sh_name)} is truncated")
        result.append(
            ExecutableRange(
                sh_addr & 0xFFFFFFFF,
                data[sh_offset : sh_offset + sh_size],
                name_at(sh_name),
            )
        )
    return result


def load_ghidra_functions(path: Path | None) -> list[GhidraFunction]:
    if path is None:
        return []
    raw = path.read_bytes()
    functions = [
        GhidraFunction(
            int(match.group(3), 16),
            int(match.group(4), 16),
            match.group(2).decode("utf-8", errors="replace"),
        )
        for match in FUNCTION_RE.finditer(raw)
    ]
    functions.sort(key=lambda item: (item.start, item.end))
    return functions


def find_ghidra_function(
    functions: list[GhidraFunction], address: int
) -> GhidraFunction | None:
    if not functions:
        return None
    starts = [item.start for item in functions]
    index = bisect.bisect_right(starts, address) - 1
    if index < 0:
        return None
    item = functions[index]
    return item if item.start <= address <= item.end else None


def load_attribute_helper() -> ModuleType | None:
    if not ATTRIBUTE_HELPER.is_file():
        return None
    spec = importlib.util.spec_from_file_location("rmcp01_attribute", ATTRIBUTE_HELPER)
    if spec is None or spec.loader is None:
        return None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def resolve_doldecomp(helper: ModuleType | None, requested: Path | None) -> Path | None:
    if helper is None:
        return None
    return helper.find_doldecomp_dir(requested)


def attribute_callsite(
    address: int,
    image_range: str,
    target: int,
    target_symbol: str,
    ghidra_functions: list[GhidraFunction],
    helper: ModuleType | None,
    doldecomp: Path | None,
) -> Callsite:
    ghidra = find_ghidra_function(ghidra_functions, address)
    public = None
    if helper is not None and doldecomp is not None:
        public = helper.attribute(address, doldecomp, None, None)

    confidence_parts = ["direct-ppc-bl"]
    if ghidra is not None:
        confidence_parts.append("ghidra-function-range")
    if public is not None and public.confidence != "unresolved":
        confidence_parts.append(public.confidence)

    return Callsite(
        callsite=f"0x{address:08X}",
        target=f"0x{target:08X}",
        target_symbol=target_symbol,
        image_range=image_range,
        ghidra_function=ghidra.name if ghidra else None,
        ghidra_function_range=(
            f"0x{ghidra.start:08X}..0x{ghidra.end:08X}" if ghidra else None
        ),
        module=public.module if public is not None else None,
        object_path=public.object_path if public is not None else None,
        public_symbol=public.symbol if public is not None else None,
        public_source=public.source_path if public is not None else None,
        confidence="+".join(confidence_parts),
    )


def parse_targets(values: list[str] | None) -> dict[int, str]:
    if not values:
        return dict(DEFAULT_TARGETS)
    result: dict[int, str] = {}
    for value in values:
        if "=" in value:
            address_text, symbol = value.split("=", 1)
        else:
            address_text, symbol = value, value
        address = int(address_text, 0)
        result[address] = symbol
    return result


def print_human(callsites: list[Callsite], targets: dict[int, str]) -> None:
    print("RMCP01 direct GX texture callsite scan")
    print("targets:")
    for address, symbol in targets.items():
        print(f"  0x{address:08X}  {symbol}")
    print()
    if not callsites:
        print("No matching direct PPC branch-and-link callsites found.")
        return

    for item in callsites:
        caller = item.ghidra_function or item.public_symbol or "<unresolved>"
        public = item.public_source or item.object_path or "<unresolved>"
        print(
            f"{item.callsite} -> {item.target} {item.target_symbol} "
            f"caller={caller} image={item.image_range}"
        )
        print(
            f"    module={item.module or '<unknown>'} public={public} "
            f"confidence={item.confidence}"
        )
    print()
    print(
        "These are exact direct binary callsites, not proof of runtime execution. "
        "Hardware logs remain authoritative for observed descriptor values."
    )


def make_test_elf(path: Path, text_address: int, words: list[int]) -> None:
    """Create a tiny big-endian ELF32 with one executable .text section."""
    text = b"".join(struct.pack(">I", word) for word in words)
    shstr = b"\x00.text\x00.shstrtab\x00"
    elf_header_size = 52
    text_offset = 0x100
    shstr_offset = text_offset + len(text)
    shoff = (shstr_offset + len(shstr) + 3) & ~3
    shentsize = 40
    shnum = 3
    header = struct.pack(
        ">16sHHIIIIIHHHHHH",
        b"\x7fELF\x01\x02\x01" + b"\x00" * 9,
        2,
        20,
        1,
        text_address,
        0,
        shoff,
        0,
        elf_header_size,
        0,
        0,
        shentsize,
        shnum,
        2,
    )
    null = b"\x00" * 40
    text_sh = struct.pack(
        ">IIIIIIIIII",
        1,
        1,
        0x6,
        text_address,
        text_offset,
        len(text),
        0,
        0,
        4,
        0,
    )
    shstr_sh = struct.pack(
        ">IIIIIIIIII",
        7,
        3,
        0,
        0,
        shstr_offset,
        len(shstr),
        0,
        0,
        1,
        0,
    )
    blob = bytearray(shoff + shentsize * shnum)
    blob[: len(header)] = header
    blob[text_offset : text_offset + len(text)] = text
    blob[shstr_offset : shstr_offset + len(shstr)] = shstr
    blob[shoff : shoff + 40] = null
    blob[shoff + 40 : shoff + 80] = text_sh
    blob[shoff + 80 : shoff + 120] = shstr_sh
    path.write_bytes(blob)


def self_test() -> None:
    call = 0x80010000
    target = 0x80170A4C
    instruction = encode_bl(call, target)
    assert ppc_direct_branch_target(call, instruction) == target
    assert instruction & 1
    assert ppc_direct_branch_target(call, 0x60000000) is None

    raw = ExecutableRange(call, struct.pack(">II", instruction, 0x60000000), "raw")
    hits = scan_ranges([raw], {target: "GXInitTexObjLOD"})
    assert hits == [(call, target, "raw")]

    with tempfile.TemporaryDirectory(prefix="rmcp01-gx-scan-") as temp:
        elf = Path(temp) / "test.elf"
        make_test_elf(elf, call, [instruction, 0x60000000])
        ranges = elf_ranges(elf)
        assert len(ranges) == 1
        assert ranges[0].address == call
        assert scan_ranges(ranges, {target: "GXInitTexObjLOD"}) == [
            (call, target, ".text")
        ]

        xml = Path(temp) / "test.xml"
        xml.write_text(
            '<FUNCTION ENTRY_POINT="8000fff0" NAME="Caller" LIBRARY_FUNCTION="n">\n'
            '  <ADDRESS_RANGE START="8000fff0" END="80010040" />\n'
            "</FUNCTION>\n",
            encoding="utf-8",
        )
        functions = load_ghidra_functions(xml)
        found = find_ghidra_function(functions, call)
        assert found is not None and found.name == "Caller"

    print("RMCP01 GX texture callsite scanner self-test: PASS")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Scan a user-owned RMCP01 MEM1 image or DTK-merged ELF for direct "
            "PPC calls to GX texture-object functions."
        )
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--elf", type=Path, help="DTK-merged RMCP01 ELF")
    source.add_argument("--mem1", type=Path, help="raw MEM1 image")
    parser.add_argument(
        "--mem1-base", type=lambda value: int(value, 0), default=0x80000000
    )
    parser.add_argument(
        "--target",
        action="append",
        help="target as ADDRESS or ADDRESS=NAME; repeatable",
    )
    parser.add_argument("--doldecomp", type=Path)
    parser.add_argument(
        "--ghidra-xml",
        type=Path,
        help="optional public MKW-SP pal.raw.xml for broader function names",
    )
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.elf is None and args.mem1 is None:
        parser.error("provide --elf or --mem1 unless --self-test is used")

    targets = parse_targets(args.target)
    try:
        ranges = (
            elf_ranges(args.elf.expanduser().resolve())
            if args.elf is not None
            else raw_ranges(args.mem1.expanduser().resolve(), args.mem1_base)
        )
        ghidra = load_ghidra_functions(
            args.ghidra_xml.expanduser().resolve() if args.ghidra_xml else None
        )
    except (OSError, ValueError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    helper = load_attribute_helper()
    requested = args.doldecomp.expanduser().resolve() if args.doldecomp else None
    doldecomp = resolve_doldecomp(helper, requested)

    raw_hits = scan_ranges(ranges, targets)
    callsites = [
        attribute_callsite(
            address,
            image_range,
            target,
            targets[target],
            ghidra,
            helper,
            doldecomp,
        )
        for address, target, image_range in raw_hits
    ]
    callsites.sort(key=lambda item: (int(item.callsite, 16), item.target))

    if args.json:
        print(
            json.dumps(
                {
                    "callsites": [asdict(item) for item in callsites],
                    "count": len(callsites),
                    "targets": {
                        f"0x{address:08X}": symbol
                        for address, symbol in targets.items()
                    },
                    "policy": (
                        "exact direct binary callsites are static evidence only; "
                        "hardware remains authoritative for runtime descriptors"
                    ),
                },
                indent=2,
                sort_keys=True,
            )
        )
    else:
        print_human(callsites, targets)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
