"""Scan local RMCP01 direct-call coverage before a Switch hardware run.

The local translated product is user-owned/game-derived and is never committed.
This helper only reads local generated source text and reports guest addresses.
It can also correlate the runtime Discovery NRO's first-hit trace.
"""

from __future__ import annotations

import argparse
import json
import re
import tempfile
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

DIRECT_RE = re.compile(r"InvokeDirectCpu<0x([0-9A-Fa-f]+)u?>")
NATIVE_RE = re.compile(r"KnownNativeCpuCall<0x([0-9A-Fa-f]+)u?>")
TRANSLATED_RE = re.compile(
    r"(?:KnownTranslatedCpuCall<0x([0-9A-Fa-f]+)u?>|"
    r"MKW_TRANSLATED_TRAIT\(([0-9A-Fa-f]+))"
)
TRACE_RE = re.compile(
    r"^\[(\d+)\].*?target=0x([0-9A-Fa-f]+).*?lr=0x([0-9A-Fa-f]+)",
    re.MULTILINE,
)
TEXT_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp", ".inc"}


@dataclass
class Target:
    address: str
    references: int
    coverage: str
    runtime_seen: bool
    runtime_order: int | None
    runtime_lr: str | None
    sample_files: list[str]


def source_files(root: Path) -> list[Path]:
    if not root.exists():
        return []
    if root.is_file():
        return [root] if root.suffix.lower() in TEXT_SUFFIXES else []
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in TEXT_SUFFIXES
    )


def scan_regex(roots: list[Path], pattern: re.Pattern[str]) -> tuple[set[int], dict[int, Counter[str]]]:
    addresses: set[int] = set()
    refs: dict[int, Counter[str]] = defaultdict(Counter)
    for root in roots:
        for path in source_files(root):
            text = path.read_text(encoding="utf-8", errors="replace")
            for match in pattern.finditer(text):
                groups = [group for group in match.groups() if group is not None]
                if not groups:
                    continue
                address = int(groups[0], 16)
                addresses.add(address)
                try:
                    relative = path.relative_to(REPO_ROOT).as_posix()
                except ValueError:
                    relative = path.as_posix()
                refs[address][relative] += 1
    return addresses, refs


def scan_trace(path: Path | None) -> dict[int, tuple[int, int]]:
    if path is None:
        return {}
    text = path.read_text(encoding="utf-8", errors="replace")
    seen: dict[int, tuple[int, int]] = {}
    for match in TRACE_RE.finditer(text):
        order = int(match.group(1), 10)
        address = int(match.group(2), 16)
        lr = int(match.group(3), 16)
        seen.setdefault(address, (order, lr))
    return seen


def build_report(
    repo_root: Path,
    product_roots: list[Path],
    trace: Path | None,
) -> tuple[list[Target], dict[str, int]]:
    native_roots = [repo_root / "include", repo_root / "source"]
    native, _ = scan_regex(native_roots, NATIVE_RE)

    translated_roots = product_roots + [
        repo_root / "local-product-support",
        repo_root / "local-execution-support",
    ]
    translated, _ = scan_regex(translated_roots, TRANSLATED_RE)
    direct, direct_refs = scan_regex(product_roots, DIRECT_RE)
    runtime = scan_trace(trace)

    targets: list[Target] = []
    for address in direct:
        if address in native:
            coverage = "native"
        elif address in translated:
            coverage = "translated"
        else:
            coverage = "missing"

        trace_hit = runtime.get(address)
        sample_files = [
            name
            for name, _ in direct_refs[address].most_common(4)
        ]
        targets.append(
            Target(
                address=f"0x{address:08X}",
                references=sum(direct_refs[address].values()),
                coverage=coverage,
                runtime_seen=trace_hit is not None,
                runtime_order=trace_hit[0] if trace_hit else None,
                runtime_lr=f"0x{trace_hit[1]:08X}" if trace_hit else None,
                sample_files=sample_files,
            )
        )

    targets.sort(
        key=lambda item: (
            item.coverage != "missing",
            not item.runtime_seen,
            item.runtime_order if item.runtime_order is not None else 1 << 30,
            -item.references,
            item.address,
        )
    )
    summary = {
        "direct_targets": len(direct),
        "native": sum(item.coverage == "native" for item in targets),
        "translated": sum(item.coverage == "translated" for item in targets),
        "missing": sum(item.coverage == "missing" for item in targets),
        "runtime_seen": sum(item.runtime_seen for item in targets),
        "runtime_seen_missing": sum(
            item.runtime_seen and item.coverage == "missing"
            for item in targets
        ),
    }
    return targets, summary


def print_human(targets: list[Target], summary: dict[str, int]) -> None:
    print("RMCP01 local direct-dispatch coverage scan")
    print("========================================")
    for key in (
        "direct_targets",
        "native",
        "translated",
        "missing",
        "runtime_seen",
        "runtime_seen_missing",
    ):
        print(f"{key:20s}: {summary[key]}")
    print()
    print("Missing direct targets (static product scan):")
    missing = [item for item in targets if item.coverage == "missing"]
    if not missing:
        print("  <none>")
        return
    for item in missing:
        runtime = (
            f"runtime=#{item.runtime_order} lr={item.runtime_lr}"
            if item.runtime_seen
            else "runtime=not-seen"
        )
        files = ", ".join(item.sample_files) if item.sample_files else "-"
        print(
            f"  {item.address} refs={item.references:<4d} "
            f"{runtime} files={files}"
        )


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="rmcp01-dispatch-scan-") as temp:
        root = Path(temp)
        repo = root / "repo"
        product = repo / "local-product" / "generated"
        include = repo / "include"
        product.mkdir(parents=True)
        include.mkdir(parents=True)

        (product / "calls.cpp").write_text(
            "InvokeDirectCpu<0x80001000u>(ctx);\n"
            "InvokeDirectCpu<0x80002000u>(ctx);\n"
            "InvokeDirectCpu<0x80002000u>(ctx);\n"
            "InvokeDirectCpu<0x80003000u>(ctx);\n",
            encoding="utf-8",
        )
        (product / "traits.hpp").write_text(
            "MKW_TRANSLATED_TRAIT(80002000, func_80002000, 0);\n",
            encoding="utf-8",
        )
        (include / "native.hpp").write_text(
            "template <> struct KnownNativeCpuCall<0x80001000u> {};\n",
            encoding="utf-8",
        )
        trace = root / "trace.txt"
        trace.write_text(
            "[0001] dispatch=1 target=0x80003000 pc=0x1 lr=0x8000ABCD\n",
            encoding="utf-8",
        )

        global REPO_ROOT
        old_root = REPO_ROOT
        REPO_ROOT = repo
        try:
            targets, summary = build_report(repo, [product], trace)
        finally:
            REPO_ROOT = old_root

        assert summary["direct_targets"] == 3
        assert summary["native"] == 1
        assert summary["translated"] == 1
        assert summary["missing"] == 1
        missing = next(item for item in targets if item.coverage == "missing")
        assert missing.address == "0x80003000"
        assert missing.runtime_seen
        assert missing.runtime_order == 1
        assert missing.runtime_lr == "0x8000ABCD"

    print("RMCP01 local dispatch coverage scanner self-test: PASS")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Scan the local generated RMCP01 product for every InvokeDirectCpu "
            "target and compare it with Switch native/translated coverage."
        )
    )
    parser.add_argument(
        "--product-root",
        action="append",
        type=Path,
        help=(
            "local translated source root; may be repeated "
            "(default: local-product/generated)"
        ),
    )
    parser.add_argument(
        "--trace",
        type=Path,
        help="optional fast-track-discovery-targets.txt to correlate runtime hits",
    )
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--missing-only", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0

    roots = args.product_root or [REPO_ROOT / "local-product" / "generated"]
    roots = [path.expanduser().resolve() for path in roots]
    missing_roots = [path for path in roots if not path.exists()]
    if missing_roots:
        parser.error(
            "missing product root(s): "
            + ", ".join(str(path) for path in missing_roots)
        )

    trace = args.trace.expanduser().resolve() if args.trace else None
    if trace is not None and not trace.is_file():
        parser.error(f"trace does not exist: {trace}")

    targets, summary = build_report(REPO_ROOT, roots, trace)
    if args.missing_only:
        targets = [item for item in targets if item.coverage == "missing"]

    if args.json:
        print(
            json.dumps(
                {
                    "summary": summary,
                    "targets": [asdict(item) for item in targets],
                    "policy": (
                        "static coverage + runtime first-hit evidence; "
                        "unknown/stateful boundaries remain hard stops"
                    ),
                },
                indent=2,
                sort_keys=True,
            )
        )
    else:
        print_human(targets, summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
