#!/usr/bin/env python3
"""Attribute a PAL RMCP01 guest address using public decompilation metadata."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DOLDECOMP_DIR = REPO_ROOT / ".deps" / "analysis" / "mkw"
DOLDECOMP_REPO = "https://github.com/doldecomp/mkw.git"

SPLIT_RE = re.compile(
    r"^\s*(?P<section>\.\S+)\s+start:0x(?P<start>[0-9A-Fa-f]+)"
    r"\s+end:0x(?P<end>[0-9A-Fa-f]+)"
)
SYMBOL_PAL_RE = re.compile(
    r"//\s*Symbol:\s*(?P<symbol>[^\n\r]+)\s*"
    r"//\s*PAL:\s*0x(?P<start>[0-9A-Fa-f]+)"
    r"\.\.0x(?P<end>[0-9A-Fa-f]+)",
    re.MULTILINE,
)
PAL_RANGE_RE = re.compile(
    r"//\s*PAL:\s*0x(?P<start>[0-9A-Fa-f]+)"
    r"\.\.0x(?P<end>[0-9A-Fa-f]+)"
)
BLOB_RE = re.compile(
    r"MARK_BINARY_BLOB\(\s*(?P<symbol>[^,\s]+)\s*,\s*"
    r"0x(?P<start>[0-9A-Fa-f]+)\s*,\s*"
    r"0x(?P<end>[0-9A-Fa-f]+)\s*\)"
)
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}


@dataclass(frozen=True)
class SplitMatch:
    module: str
    object_path: str
    section: str
    start: int
    end: int

    @property
    def size(self) -> int:
        return self.end - self.start


@dataclass(frozen=True)
class SymbolMatch:
    symbol: str | None
    source_path: str
    start: int
    end: int
    line: int

    @property
    def size(self) -> int:
        return self.end - self.start


@dataclass
class Attribution:
    address: str
    doldecomp_dir: str
    doldecomp_revision: str | None
    module: str | None
    section: str | None
    object_path: str | None
    object_range: str | None
    object_offset: str | None
    symbol: str | None
    symbol_range: str | None
    symbol_offset: str | None
    source_path: str | None
    source_line: int | None
    confidence: str
    dtk_available: bool
    dtk_binary: str | None
    dtk_follow_up: list[str]
    notes: list[str]


def parse_address(value: str) -> int:
    try:
        address = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            f"invalid address {value!r}; use e.g. 0x80194290"
        ) from exc

    if not 0 <= address <= 0xFFFFFFFF:
        raise argparse.ArgumentTypeError("address must fit in an unsigned 32-bit value")
    return address


def run_git(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def prepare_doldecomp(checkout: Path) -> None:
    if shutil.which("git") is None:
        raise RuntimeError("git is required for --fetch")

    if checkout.exists():
        if not (checkout / ".git").exists():
            raise RuntimeError(
                f"{checkout} exists but is not a git checkout; "
                "choose another --doldecomp directory"
            )
        run_git(["fetch", "--depth", "1", "origin", "main"], cwd=checkout)
        run_git(["merge", "--ff-only", "origin/main"], cwd=checkout)
        return

    checkout.parent.mkdir(parents=True, exist_ok=True)
    run_git(
        [
            "clone",
            "--depth",
            "1",
            "--branch",
            "main",
            DOLDECOMP_REPO,
            str(checkout),
        ]
    )


def candidate_doldecomp_dirs(explicit: Path | None) -> Iterable[Path]:
    if explicit is not None:
        yield explicit
        return

    env_dir = os.environ.get("MKW_DOLDECOMP_DIR")
    if env_dir:
        yield Path(env_dir).expanduser()

    yield DEFAULT_DOLDECOMP_DIR
    yield REPO_ROOT.parent / "mkw"
    yield REPO_ROOT.parent / "doldecomp-mkw"


def find_doldecomp_dir(explicit: Path | None) -> Path | None:
    for candidate in candidate_doldecomp_dirs(explicit):
        resolved = candidate.expanduser().resolve()
        if (resolved / "config" / "RMCP01" / "config.yml").is_file():
            return resolved
    return None


def git_revision(checkout: Path) -> str | None:
    if shutil.which("git") is None or not (checkout / ".git").exists():
        return None
    try:
        return run_git(["rev-parse", "HEAD"], cwd=checkout).stdout.strip()
    except subprocess.CalledProcessError:
        return None


def parse_splits(path: Path, module: str, address: int) -> list[SplitMatch]:
    matches: list[SplitMatch] = []
    current_object: str | None = None

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.rstrip()
        stripped = line.strip()

        if (
            stripped.endswith(":")
            and not line[:1].isspace()
            and stripped != "Sections:"
        ):
            current_object = stripped[:-1]
            continue

        match = SPLIT_RE.match(line)
        if match is None or current_object is None:
            continue

        start = int(match.group("start"), 16)
        end = int(match.group("end"), 16)
        if start <= address < end:
            matches.append(
                SplitMatch(
                    module=module,
                    object_path=current_object,
                    section=match.group("section"),
                    start=start,
                    end=end,
                )
            )

    return matches


def split_matches(checkout: Path, address: int) -> list[SplitMatch]:
    config = checkout / "config" / "RMCP01"
    candidates = [
        (config / "splits.txt", "main.dol"),
        (config / "module" / "splits.txt", "StaticR.rel"),
    ]

    matches: list[SplitMatch] = []
    for path, module in candidates:
        if path.is_file():
            matches.extend(parse_splits(path, module, address))

    return sorted(
        matches,
        key=lambda item: (
            item.section != ".text",
            item.size,
            item.module,
            item.object_path,
        ),
    )


def source_files(checkout: Path) -> Iterable[Path]:
    for root_name in ("src", "lib"):
        root = checkout / root_name
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                yield path


def source_symbol_matches(checkout: Path, address: int) -> list[SymbolMatch]:
    found: dict[tuple[str, int, int, str | None], SymbolMatch] = {}

    for path in source_files(checkout):
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        relative = path.relative_to(checkout).as_posix()

        for regex, has_symbol in (
            (SYMBOL_PAL_RE, True),
            (BLOB_RE, True),
            (PAL_RANGE_RE, False),
        ):
            for match in regex.finditer(text):
                start = int(match.group("start"), 16)
                end = int(match.group("end"), 16)
                if not start <= address < end:
                    continue

                symbol = match.group("symbol").strip() if has_symbol else None
                line = text.count("\n", 0, match.start()) + 1
                key = (relative, start, end, symbol)
                found[key] = SymbolMatch(
                    symbol=symbol,
                    source_path=relative,
                    start=start,
                    end=end,
                    line=line,
                )

    return sorted(
        found.values(),
        key=lambda item: (
            item.symbol is None,
            item.start != address,
            item.size,
            item.source_path,
        ),
    )


def resolve_dtk(explicit: str | None) -> str | None:
    candidate = explicit or os.environ.get("MKW_DTK_BIN")
    if candidate:
        path = Path(candidate).expanduser()
        if path.is_file():
            return str(path.resolve())
        resolved = shutil.which(candidate)
        if resolved:
            return resolved
        return None
    return shutil.which("dtk")


def dtk_follow_up(
    dtk: str | None,
    module: str | None,
    disc: Path | None,
) -> list[str]:
    if dtk is None:
        return []

    quoted_dtk = json.dumps(dtk)
    if disc is None:
        if module == "StaticR.rel":
            return [
                f"{quoted_dtk} rel info /path/to/StaticR.rel",
                f"{quoted_dtk} rel merge /path/to/main.dol "
                "/path/to/StaticR.rel -o /tmp/rmcp01-merged.elf",
            ]
        return [f"{quoted_dtk} dol info /path/to/main.dol"]

    disc_text = str(disc.expanduser().resolve())
    if module == "StaticR.rel":
        rel_vfs = f"{disc_text}:files/rel/StaticR.rel"
        return [f"{quoted_dtk} rel info {json.dumps(rel_vfs)}"]

    dol_vfs = f"{disc_text}:sys/main.dol"
    return [f"{quoted_dtk} dol info {json.dumps(dol_vfs)}"]


def attribute(
    address: int,
    checkout: Path,
    dtk: str | None,
    disc: Path | None,
) -> Attribution:
    splits = split_matches(checkout, address)
    symbols = source_symbol_matches(checkout, address)

    split = splits[0] if splits else None
    symbol = symbols[0] if symbols else None

    notes: list[str] = []
    if len(splits) > 1:
        notes.append(
            f"{len(splits)} split ranges matched; showing the narrowest .text-first match"
        )
    if len(symbols) > 1:
        notes.append(
            f"{len(symbols)} source ranges matched; showing the narrowest exact-start-first match"
        )

    if symbol is not None and symbol.symbol is not None:
        confidence = "source-symbol-range"
    elif split is not None:
        confidence = "doldecomp-split-range"
    elif symbol is not None:
        confidence = "source-pal-range"
    else:
        confidence = "unresolved"
        notes.append(
            "No public doldecomp split/source range matched this address. "
            "Use DTK on the local RMCP01 binary and pinned WiiCompiled semantics."
        )

    module = split.module if split is not None else None

    return Attribution(
        address=f"0x{address:08X}",
        doldecomp_dir=str(checkout),
        doldecomp_revision=git_revision(checkout),
        module=module,
        section=split.section if split is not None else None,
        object_path=split.object_path if split is not None else None,
        object_range=(
            f"0x{split.start:08X}..0x{split.end:08X}" if split is not None else None
        ),
        object_offset=f"+0x{address - split.start:X}" if split is not None else None,
        symbol=symbol.symbol if symbol is not None else None,
        symbol_range=(
            f"0x{symbol.start:08X}..0x{symbol.end:08X}"
            if symbol is not None
            else None
        ),
        symbol_offset=f"+0x{address - symbol.start:X}" if symbol is not None else None,
        source_path=symbol.source_path if symbol is not None else None,
        source_line=symbol.line if symbol is not None else None,
        confidence=confidence,
        dtk_available=dtk is not None,
        dtk_binary=dtk,
        dtk_follow_up=dtk_follow_up(dtk, module, disc),
        notes=notes,
    )


def print_human(result: Attribution) -> None:
    print("RMCP01 address attribution")
    print(f"address             : {result.address}")
    print(f"module              : {result.module or '<unresolved>'}")
    print(f"section             : {result.section or '<unresolved>'}")
    print(f"translation unit    : {result.object_path or '<unresolved>'}")
    print(f"translation range   : {result.object_range or '<unresolved>'}")
    print(f"TU offset           : {result.object_offset or '<unresolved>'}")
    print(f"symbol              : {result.symbol or '<unresolved>'}")
    print(f"symbol range        : {result.symbol_range or '<unresolved>'}")
    print(f"symbol offset       : {result.symbol_offset or '<unresolved>'}")
    if result.source_path is not None:
        suffix = f":{result.source_line}" if result.source_line is not None else ""
        print(f"source              : {result.source_path}{suffix}")
    else:
        print("source              : <unresolved>")
    print(f"confidence          : {result.confidence}")
    print(f"doldecomp revision  : {result.doldecomp_revision or '<unknown>'}")
    print(f"dtk                 : {result.dtk_binary or '<not found>'}")

    if result.dtk_follow_up:
        print("\nDTK follow-up:")
        for command in result.dtk_follow_up:
            print(f"  {command}")

    if result.notes:
        print("\nNotes:")
        for note in result.notes:
            print(f"  - {note}")


def write_fixture(root: Path) -> None:
    config = root / "config" / "RMCP01"
    module = config / "module"
    source = root / "src" / "game"
    lib = root / "lib" / "rvl" / "gx"
    module.mkdir(parents=True)
    source.mkdir(parents=True)
    lib.mkdir(parents=True)

    (config / "config.yml").write_text(
        "object: sys/main.dol\nmodules:\n- object: files/rel/StaticR.rel\n",
        encoding="utf-8",
    )
    (config / "splits.txt").write_text(
        "Sections:\n"
        "\t.text type:code align:32\n\n"
        "game/Test.cpp:\n"
        "\t.text start:0x80001000 end:0x80001100\n",
        encoding="utf-8",
    )
    (module / "splits.txt").write_text(
        "Sections:\n"
        "\t.text type:code align:4 vaddr:0x80500000\n\n"
        "game/Rel.cpp:\n"
        "\t.text start:0x80501000 end:0x80501200\n",
        encoding="utf-8",
    )
    (source / "Test.cpp").write_text(
        "// Symbol: TestFunction\n"
        "// PAL: 0x80001020..0x80001040\n"
        "MARK_BINARY_BLOB(TestFunction, 0x80001020, 0x80001040);\n",
        encoding="utf-8",
    )
    (lib / "Rel.cpp").write_text(
        "// Symbol: RelFunction\n"
        "// PAL: 0x80501040..0x80501080\n"
        "MARK_BINARY_BLOB(RelFunction, 0x80501040, 0x80501080);\n",
        encoding="utf-8",
    )


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="rmcp01-attribution-") as temp:
        root = Path(temp)
        write_fixture(root)

        main = attribute(0x80001020, root, None, None)
        assert main.module == "main.dol"
        assert main.object_path == "game/Test.cpp"
        assert main.symbol == "TestFunction"
        assert main.symbol_offset == "+0x0"

        rel = attribute(0x80501050, root, None, None)
        assert rel.module == "StaticR.rel"
        assert rel.object_path == "game/Rel.cpp"
        assert rel.symbol == "RelFunction"
        assert rel.symbol_offset == "+0x10"

        missing = attribute(0x81234567, root, None, None)
        assert missing.module is None
        assert missing.confidence == "unresolved"

    print("RMCP01 attribution self-test: PASS")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Attribute an RMCP01 PAL guest address using local doldecomp/mkw "
            "metadata, with optional DTK follow-up commands."
        )
    )
    parser.add_argument(
        "address",
        nargs="?",
        type=parse_address,
        help="guest address, e.g. 0x80194290",
    )
    parser.add_argument(
        "--doldecomp",
        type=Path,
        help=(
            "path to a local doldecomp/mkw checkout; defaults to "
            "MKW_DOLDECOMP_DIR, .deps/analysis/mkw, ../mkw, then ../doldecomp-mkw"
        ),
    )
    parser.add_argument(
        "--fetch",
        action="store_true",
        help=(
            "clone/update public doldecomp/mkw metadata before attribution; "
            "network access is never used unless this flag is present"
        ),
    )
    parser.add_argument(
        "--dtk",
        help="path/name of the dtk binary; defaults to MKW_DTK_BIN or PATH",
    )
    parser.add_argument(
        "--disc",
        type=Path,
        help="optional local RMCP01 disc image path used only for DTK command hints",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="emit machine-readable JSON",
    )
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="run Nintendo-data-free parser self-tests and exit",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return 0

    if args.address is None:
        parser.error("address is required unless --self-test is used")

    requested = args.doldecomp.expanduser().resolve() if args.doldecomp else None
    if args.fetch:
        fetch_target = requested or DEFAULT_DOLDECOMP_DIR
        try:
            prepare_doldecomp(fetch_target)
        except (RuntimeError, subprocess.CalledProcessError) as exc:
            print(f"error: failed to prepare doldecomp/mkw: {exc}", file=sys.stderr)
            return 2

    checkout = find_doldecomp_dir(requested)
    if checkout is None:
        print(
            "error: doldecomp/mkw checkout not found.\n"
            "Run once with --fetch, set MKW_DOLDECOMP_DIR, or pass "
            "--doldecomp /path/to/mkw.",
            file=sys.stderr,
        )
        return 2

    dtk = resolve_dtk(args.dtk)
    result = attribute(args.address, checkout, dtk, args.disc)

    if args.json:
        print(json.dumps(asdict(result), indent=2, sort_keys=True))
    else:
        print_human(result)

    return 0 if result.confidence != "unresolved" else 1


if __name__ == "__main__":
    raise SystemExit(main())
