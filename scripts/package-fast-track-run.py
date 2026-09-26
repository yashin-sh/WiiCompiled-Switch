"""Bundle fast-track text diagnostics into a compact upload artifact."""

from __future__ import annotations

import argparse
import hashlib
import tempfile
import zipfile
from pathlib import Path

VERBOSE_ONLY = {
    "fast-track-os-sleep-events.txt",
    "fast-track-os-receive-message-frontier.txt",
    "fast-track-thread-events.txt",
    "fast-track-post-main-trace.txt",
    "fast-track-post-video-trace.txt",
    "fast-track-os-message-events.txt",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def discover(source: Path) -> list[Path]:
    return sorted(path for path in source.glob("*.txt") if path.is_file())


def select(files: list[Path], full: bool) -> tuple[list[Path], list[Path]]:
    if full:
        return files, []
    included = [path for path in files if path.name not in VERBOSE_ONLY]
    excluded = [path for path in files if path.name in VERBOSE_ONLY]
    return included, excluded


def build_report(files: list[Path]) -> str:
    parts: list[str] = []
    for path in files:
        parts.append(f"===== {path.name} =====")
        parts.append(path.read_text(encoding="utf-8", errors="replace").rstrip())
        parts.append("")
    return "\n".join(parts).rstrip() + "\n"


def build_manifest(
    source: Path,
    included: list[Path],
    excluded: list[Path],
    total_files: list[Path],
    full: bool,
) -> str:
    total_bytes = sum(path.stat().st_size for path in total_files)
    included_bytes = sum(path.stat().st_size for path in included)
    profile = "full" if full else "compact"

    lines = [
        "WiiCompiled-Switch fast-track log bundle",
        f"profile={profile}",
        f"source={source}",
        f"source_files={len(total_files)}",
        f"included_files={len(included)}",
        f"excluded_files={len(excluded)}",
        f"source_bytes={total_bytes}",
        f"included_bytes={included_bytes}",
        "",
        "[included]",
    ]
    for path in included:
        lines.append(
            f"{path.name}\t{path.stat().st_size}\tsha256={sha256(path)}"
        )

    if excluded:
        lines.extend(["", "[excluded-verbose]"])
        for path in excluded:
            lines.append(f"{path.name}\t{path.stat().st_size}")

    return "\n".join(lines) + "\n"


def bundle(source: Path, output: Path, full: bool, raw: bool) -> tuple[int, int, int]:
    files = discover(source)
    if not files:
        raise ValueError(f"no .txt diagnostics found in {source}")

    included, excluded = select(files, full)
    report = build_report(included)
    manifest = build_manifest(source, included, excluded, files, full)

    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("fast-track-report.txt", report)
        archive.writestr("manifest.txt", manifest)
        if raw:
            for path in included:
                archive.write(path, arcname=f"raw/{path.name}")

    return len(files), len(included), sum(path.stat().st_size for path in included)


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="fast-track-bundle-") as temp:
        root = Path(temp)
        (root / "fast-track-dispatch-blocker.txt").write_text(
            "target : 0x80170A4C\n", encoding="utf-8"
        )
        (root / "rendered-fast-track-graphics.txt").write_text(
            "present successes : 92\n", encoding="utf-8"
        )
        (root / "fast-track-os-sleep-events.txt").write_text(
            "verbose\n", encoding="utf-8"
        )

        compact = root / "compact.zip"
        total, included, _ = bundle(root, compact, full=False, raw=False)
        assert total == 3
        assert included == 2
        with zipfile.ZipFile(compact) as archive:
            names = set(archive.namelist())
            assert names == {"fast-track-report.txt", "manifest.txt"}
            report = archive.read("fast-track-report.txt").decode()
            assert "fast-track-dispatch-blocker.txt" in report
            assert "fast-track-os-sleep-events.txt" not in report

        full = root / "full.zip"
        _, included_full, _ = bundle(root, full, full=True, raw=True)
        assert included_full == 3
        with zipfile.ZipFile(full) as archive:
            assert "raw/fast-track-os-sleep-events.txt" in archive.namelist()

    print("fast-track log bundle self-test: PASS")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Bundle WiiCompiled-Switch fast-track .txt diagnostics into one "
            "compact report ZIP."
        )
    )
    parser.add_argument(
        "source",
        nargs="?",
        type=Path,
        help="directory containing the NRO-generated .txt diagnostics",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="output ZIP path (default: <source>/fast-track-run-compact.zip)",
    )
    parser.add_argument(
        "--full",
        action="store_true",
        help="include verbose historical traces in the consolidated report",
    )
    parser.add_argument(
        "--raw",
        action="store_true",
        help="also place the selected original .txt files under raw/ in the ZIP",
    )
    parser.add_argument("--self-test", action="store_true")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return 0

    if args.source is None:
        parser.error("source directory is required unless --self-test is used")

    source = args.source.expanduser().resolve()
    if not source.is_dir():
        parser.error(f"source directory does not exist: {source}")

    default_name = "fast-track-run-full.zip" if args.full else "fast-track-run-compact.zip"
    output = (
        args.output.expanduser().resolve()
        if args.output is not None
        else source / default_name
    )

    try:
        total, included, included_bytes = bundle(
            source,
            output,
            full=args.full,
            raw=args.raw,
        )
    except (OSError, ValueError) as exc:
        print(f"error: {exc}")
        return 2

    total_bytes = sum(path.stat().st_size for path in discover(source))
    ratio = (included_bytes / total_bytes * 100.0) if total_bytes else 0.0
    print(f"created: {output}")
    print(f"source diagnostics : {total} files / {total_bytes} bytes")
    print(f"report selection   : {included} files / {included_bytes} bytes ({ratio:.1f}%)")
    print("archive contents   : fast-track-report.txt + manifest.txt")
    if args.raw:
        print("raw originals      : included under raw/")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
