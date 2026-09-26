"""Forecast likely RMCP01 boundaries from public decompilation callsites.

This is a static-analysis hinting tool. Hardware evidence still decides which
boundary may be implemented.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from types import ModuleType

REPO_ROOT = Path(__file__).resolve().parents[1]
ATTRIBUTE_HELPER = REPO_ROOT / "scripts" / "attribute-rmcp01-address.py"

CALL_RE = re.compile(r"(?<![A-Za-z0-9_:~])([A-Za-z_~][A-Za-z0-9_:~]*)\s*\(")
TARGET_RE = re.compile(r"^\s*target\s*:\s*(0x[0-9A-Fa-f]+)\s*$", re.MULTILINE)
NATIVE_RE = re.compile(r"KnownNativeCpuCall<0x([0-9A-Fa-f]+)u?>")
IGNORE_CALLS = {
    "if",
    "for",
    "while",
    "switch",
    "sizeof",
    "alignof",
    "decltype",
    "return",
    "static_cast",
    "reinterpret_cast",
    "const_cast",
    "dynamic_cast",
    "assert",
    "MARK_BINARY_BLOB",
}


@dataclass(frozen=True)
class SymbolDef:
    symbol: str
    address: int
    end: int
    source_path: str


@dataclass
class Candidate:
    symbol: str
    address: str | None
    source_path: str | None
    callsites: int
    nearest_line_distance: int
    coverage: str
    score: int
    rationale: list[str]


def load_attribute_helper() -> ModuleType:
    spec = importlib.util.spec_from_file_location("rmcp01_attribute", ATTRIBUTE_HELPER)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {ATTRIBUTE_HELPER}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def parse_blocker_target(path: Path) -> int:
    text = path.read_text(encoding="utf-8", errors="replace")
    match = TARGET_RE.search(text)
    if match is None:
        raise ValueError(f"no 'target : 0x...' line found in {path}")
    return int(match.group(1), 16)


def build_symbol_index(helper: ModuleType, checkout: Path) -> dict[str, list[SymbolDef]]:
    index: dict[str, list[SymbolDef]] = defaultdict(list)
    for path in helper.source_files(checkout):
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        relative = path.relative_to(checkout).as_posix()
        for match in helper.SYMBOL_PAL_RE.finditer(text):
            symbol = match.group("symbol").strip()
            index[symbol].append(
                SymbolDef(
                    symbol=symbol,
                    address=int(match.group("start"), 16),
                    end=int(match.group("end"), 16),
                    source_path=relative,
                )
            )
        for match in helper.BLOB_RE.finditer(text):
            symbol = match.group("symbol").strip()
            entry = SymbolDef(
                symbol=symbol,
                address=int(match.group("start"), 16),
                end=int(match.group("end"), 16),
                source_path=relative,
            )
            if entry not in index[symbol]:
                index[symbol].append(entry)
    return index


def build_native_coverage(repo_root: Path) -> set[int]:
    addresses: set[int] = set()
    include = repo_root / "include"
    if not include.is_dir():
        return addresses

    for path in include.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in {".h", ".hh", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in NATIVE_RE.finditer(text):
            addresses.add(int(match.group(1), 16))
    return addresses


def constrained_symbols(repo_root: Path) -> set[str]:
    constrained: set[str] = set()
    roots = [repo_root / "include", repo_root / "source"]
    for root in roots:
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {
                ".c",
                ".cc",
                ".cpp",
                ".h",
                ".hh",
                ".hpp",
            }:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            if "UNPROVEN_TUPLE" not in text and "exact observed" not in text.lower():
                continue
            for symbol in re.findall(r"\b(?:GX|IOS|OS|VI|WPAD|PAD)[A-Za-z0-9_]+\b", text):
                constrained.add(symbol)
    return constrained


def source_files_for_calls(checkout: Path) -> list[Path]:
    files: list[Path] = []
    for root_name in ("src", "lib"):
        root = checkout / root_name
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix.lower() in {".c", ".cc", ".cpp"}:
                files.append(path)
    return files


def find_following_calls(
    checkout: Path,
    symbol: str,
    definition_path: str | None,
    definition_line: int | None,
    window: int,
) -> tuple[Counter[str], dict[str, int]]:
    counts: Counter[str] = Counter()
    nearest: dict[str, int] = {}

    symbol_re = re.compile(rf"\b{re.escape(symbol)}\s*\(")

    for path in source_files_for_calls(checkout):
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        relative = path.relative_to(checkout).as_posix()

        for index, line in enumerate(lines):
            if symbol_re.search(line) is None:
                continue

            line_no = index + 1
            if (
                definition_path == relative
                and definition_line is not None
                and abs(line_no - definition_line) <= 4
            ):
                continue

            seen_here: set[str] = set()
            for offset in range(1, window + 1):
                next_index = index + offset
                if next_index >= len(lines):
                    break
                candidate_line = lines[next_index]
                stripped = candidate_line.strip()
                if stripped.startswith(("//", "/*")):
                    continue

                for match in CALL_RE.finditer(candidate_line):
                    candidate = match.group(1)
                    if candidate in IGNORE_CALLS or candidate == symbol:
                        continue
                    if candidate.startswith("__attribute__"):
                        continue
                    if candidate in seen_here:
                        continue
                    seen_here.add(candidate)
                    counts[candidate] += 1
                    nearest[candidate] = min(nearest.get(candidate, offset), offset)

    return counts, nearest


def best_symbol_def(
    index: dict[str, list[SymbolDef]],
    symbol: str,
) -> SymbolDef | None:
    matches = index.get(symbol)
    if not matches:
        return None
    return min(matches, key=lambda item: (item.end - item.address, item.source_path))


def coverage_for(
    symbol_def: SymbolDef | None,
    native_addresses: set[int],
    constrained: set[str],
) -> tuple[str, list[str]]:
    rationale: list[str] = []
    if symbol_def is None:
        return "unknown-symbol", ["no PAL symbol range found in public decomp"]

    address = symbol_def.address
    source_path = symbol_def.source_path

    if address in native_addresses:
        if symbol_def.symbol in constrained:
            rationale.append("native mapping exists but local bridge is hardware-constrained")
            return "mapped-native-constrained", rationale
        rationale.append("KnownNativeCpuCall mapping exists")
        return "mapped-native", rationale

    if source_path.startswith("src/"):
        rationale.append("game-side source is normally handled by translated product")
        return "translated-source", rationale

    if source_path.startswith(("lib/rvl/", "lib/nw4r/")):
        rationale.append("SDK/library symbol has no KnownNativeCpuCall mapping")
        return "native-unmapped-candidate", rationale

    rationale.append("public symbol exists but local execution class is uncertain")
    return "unclassified", rationale


def score_candidate(
    coverage: str,
    callsites: int,
    nearest_distance: int,
) -> int:
    score = max(0, 100 - nearest_distance * 5)
    score += min(30, max(0, callsites - 1) * 10)
    if coverage == "native-unmapped-candidate":
        score += 25
    elif coverage == "mapped-native-constrained":
        score += 20
    elif coverage == "mapped-native":
        score += 5
    elif coverage == "translated-source":
        score -= 20
    return score


def forecast(
    address: int,
    checkout: Path,
    helper: ModuleType,
    repo_root: Path,
    window: int,
    depth: int,
) -> tuple[object, list[Candidate]]:
    current = helper.attribute(address, checkout, None, None)
    if current.symbol is None:
        return current, []

    index = build_symbol_index(helper, checkout)
    native_addresses = build_native_coverage(repo_root)
    constrained = constrained_symbols(repo_root)
    counts, nearest = find_following_calls(
        checkout,
        current.symbol,
        current.source_path,
        current.source_line,
        window,
    )

    candidates: list[Candidate] = []
    for symbol, count in counts.items():
        symbol_def = best_symbol_def(index, symbol)
        coverage, rationale = coverage_for(symbol_def, native_addresses, constrained)
        distance = nearest[symbol]
        if distance == 1:
            rationale.append("appears immediately after the current call at a public callsite")
        else:
            rationale.append(f"nearest public callsite occurrence is +{distance} lines")
        if count > 1:
            rationale.append(f"seen after current symbol at {count} public callsites")

        candidates.append(
            Candidate(
                symbol=symbol,
                address=(
                    f"0x{symbol_def.address:08X}" if symbol_def is not None else None
                ),
                source_path=symbol_def.source_path if symbol_def is not None else None,
                callsites=count,
                nearest_line_distance=distance,
                coverage=coverage,
                score=score_candidate(coverage, count, distance),
                rationale=rationale,
            )
        )

    candidates.sort(
        key=lambda item: (
            -item.score,
            item.nearest_line_distance,
            -item.callsites,
            item.symbol,
        )
    )
    return current, candidates[:depth]


def print_human(current: object, candidates: list[Candidate]) -> None:
    print("RMCP01 frontier forecast")
    print(f"current address : {current.address}")
    print(f"current symbol  : {current.symbol or '<unresolved>'}")
    print(f"module          : {current.module or '<unresolved>'}")
    print(f"source          : {current.source_path or '<unresolved>'}")
    print()
    print("Static candidates (not hardware proof):")
    if not candidates:
        print("  <none found from public callsite context>")
        return

    for rank, candidate in enumerate(candidates, start=1):
        print(
            f"{rank:2d}. {candidate.symbol:<32} "
            f"{candidate.address or '<unknown>':<12} "
            f"{candidate.coverage:<26} score={candidate.score}"
        )
        print(
            f"    callsites={candidate.callsites} "
            f"nearest=+{candidate.nearest_line_distance} lines "
            f"source={candidate.source_path or '<unknown>'}"
        )
        for reason in candidate.rationale:
            print(f"    - {reason}")

    print()
    print(
        "Rule: this list is a static forecast only. "
        "A real-Switch durable blocker still authorizes the patch."
    )


def write_fixture(root: Path, helper: ModuleType) -> tuple[Path, Path]:
    decomp = root / "mkw"
    helper.write_fixture(decomp)

    caller = decomp / "src" / "game" / "Caller.cpp"
    caller.write_text(
        "// Symbol: Caller\n"
        "// PAL: 0x80003000..0x80003100\n"
        "void Caller() {\n"
        "    TestFunction();\n"
        "    NextNative();\n"
        "    LaterTranslated();\n"
        "}\n"
        "// Symbol: NextNative\n"
        "// PAL: 0x80002000..0x80002020\n"
        "void NextNative() {}\n"
        "// Symbol: LaterTranslated\n"
        "// PAL: 0x80004000..0x80004020\n"
        "void LaterTranslated() {}\n",
        encoding="utf-8",
    )

    repo = root / "repo"
    include = repo / "include"
    source = repo / "source"
    include.mkdir(parents=True)
    source.mkdir(parents=True)
    (include / "native.hpp").write_text(
        "template <> struct KnownNativeCpuCall<0x80002000u> {};\n",
        encoding="utf-8",
    )
    return decomp, repo


def self_test() -> None:
    helper = load_attribute_helper()
    with tempfile.TemporaryDirectory(prefix="rmcp01-forecast-") as temp:
        root = Path(temp)
        decomp, repo = write_fixture(root, helper)
        current, candidates = forecast(
            0x80001020,
            decomp,
            helper,
            repo,
            window=8,
            depth=8,
        )
        assert current.symbol == "TestFunction"
        names = [candidate.symbol for candidate in candidates]
        assert "NextNative" in names
        next_native = next(c for c in candidates if c.symbol == "NextNative")
        assert next_native.address == "0x80002000"
        assert next_native.coverage == "mapped-native"

    print("RMCP01 frontier forecast self-test: PASS")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Forecast likely RMCP01 call boundaries using public doldecomp/mkw "
            "callsite context and local Switch coverage."
        )
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument(
        "address",
        nargs="?",
        type=lambda value: int(value, 0),
        help="current guest target address, e.g. 0x80170A4C",
    )
    source.add_argument(
        "--blocker",
        type=Path,
        help="fast-track-dispatch-blocker.txt to read the current target from",
    )
    parser.add_argument("--doldecomp", type=Path)
    parser.add_argument(
        "--fetch",
        action="store_true",
        help="clone/update public doldecomp/mkw before forecasting",
    )
    parser.add_argument(
        "--window",
        type=int,
        default=16,
        help="lines after each public callsite to inspect (default: 16)",
    )
    parser.add_argument(
        "--depth",
        type=int,
        default=12,
        help="maximum candidates to display (default: 12)",
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

    if args.window < 1 or args.depth < 1:
        parser.error("--window and --depth must be positive")

    if args.blocker is not None:
        try:
            address = parse_blocker_target(args.blocker)
        except (OSError, ValueError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 2
    elif args.address is not None:
        address = args.address
    else:
        parser.error("provide an address or --blocker")

    helper = load_attribute_helper()
    requested = args.doldecomp.expanduser().resolve() if args.doldecomp else None

    if args.fetch:
        target = requested or helper.DEFAULT_DOLDECOMP_DIR
        try:
            helper.prepare_doldecomp(target)
        except (RuntimeError, subprocess.CalledProcessError) as exc:
            print(f"error: failed to prepare doldecomp/mkw: {exc}", file=sys.stderr)
            return 2

    checkout = helper.find_doldecomp_dir(requested)
    if checkout is None:
        print(
            "error: doldecomp/mkw checkout not found; run with --fetch or "
            "pass --doldecomp.",
            file=sys.stderr,
        )
        return 2

    current, candidates = forecast(
        address,
        checkout,
        helper,
        REPO_ROOT,
        window=args.window,
        depth=args.depth,
    )

    if args.json:
        print(
            json.dumps(
                {
                    "current": asdict(current),
                    "candidates": [asdict(candidate) for candidate in candidates],
                    "policy": (
                        "static forecast only; hardware durable evidence authorizes patches"
                    ),
                },
                indent=2,
                sort_keys=True,
            )
        )
    else:
        print_human(current, candidates)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
