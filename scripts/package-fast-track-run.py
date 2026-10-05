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


def build_report(files: list[Path], contents: dict[Path, bytes] | None = None) -> str:
    parts: list[str] = []
    for path in files:
        parts.append(f"===== {path.name} =====")
        text = (
            contents[path].decode("utf-8", errors="replace")
            if contents is not None
            else path.read_text(encoding="utf-8", errors="replace")
        )
        parts.append(text.rstrip())
        parts.append("")
    return "\n".join(parts).rstrip() + "\n"


def build_manifest(
    source: Path,
    included: list[Path],
    excluded: list[Path],
    total_files: list[Path],
    full: bool,
    contents: dict[Path, bytes] | None = None,
) -> str:
    def size(path: Path) -> int:
        return len(contents[path]) if contents is not None else path.stat().st_size

    total_bytes = sum(size(path) for path in total_files)
    included_bytes = sum(size(path) for path in included)
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
        digest = (
            hashlib.sha256(contents[path]).hexdigest()
            if contents is not None
            else sha256(path)
        )
        lines.append(f"{path.name}\t{size(path)}\tsha256={digest}")

    if excluded:
        lines.extend(["", "[excluded-verbose]"])
        for path in excluded:
            lines.append(f"{path.name}\t{size(path)}")

    return "\n".join(lines) + "\n"


def bundle(
    source: Path,
    output: Path,
    full: bool,
    raw: bool,
    coverage: Path | None = None,
) -> tuple[int, int, int]:
    files = discover(source)
    if not files:
        raise ValueError(f"no .txt diagnostics found in {source}")

    input_paths = {path.resolve() for path in files}
    if coverage is not None:
        input_paths.add(coverage.resolve())
    if output.resolve() in input_paths:
        raise ValueError("output ZIP must not overwrite a diagnostic or coverage input")

    # Read each input once: report, hashes and raw members describe the same
    # bytes even if the source diagnostics are updated during packaging.
    contents = {path: path.read_bytes() for path in files}
    coverage_bytes = coverage.read_bytes() if coverage is not None else None
    included, excluded = select(files, full)
    report = build_report(included, contents)
    manifest = build_manifest(source, included, excluded, files, full, contents)
    if coverage_bytes is not None:
        manifest += (
            "\n[coverage]\nrmcp01-dispatch-coverage.json\t"
            f"{len(coverage_bytes)}\tsha256={hashlib.sha256(coverage_bytes).hexdigest()}\n"
        )

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        dir=output.parent, prefix=f".{output.name}.", suffix=".tmp", delete=False
    ) as handle:
        temporary = Path(handle.name)
    try:
        with zipfile.ZipFile(temporary, "w") as archive:

            def write_member(name: str, value: str | bytes) -> None:
                # Fixed metadata makes repeated bundles of identical captured
                # inputs deterministic and accepts old/MTP zero timestamps.
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                archive.writestr(info, value)

            write_member("fast-track-report.txt", report)
            write_member("manifest.txt", manifest)
            if raw:
                for path in included:
                    write_member(f"raw/{path.name}", contents[path])
            if coverage_bytes is not None:
                write_member("rmcp01-dispatch-coverage.json", coverage_bytes)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)

    return len(files), len(included), sum(len(contents[path]) for path in included)


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
        coverage = root / "coverage.json"
        coverage.write_text('{"missing": 1}\n', encoding="utf-8")
        _, included_full, _ = bundle(
            root,
            full,
            full=True,
            raw=True,
            coverage=coverage,
        )
        assert included_full == 3
        with zipfile.ZipFile(full) as archive:
            assert "raw/fast-track-os-sleep-events.txt" in archive.namelist()
            assert "rmcp01-dispatch-coverage.json" in archive.namelist()
            assert archive.testzip() is None

        repeated = root / "repeated.zip"
        bundle(root, repeated, full=True, raw=True, coverage=coverage)
        assert sha256(full) == sha256(repeated)
        original = (root / "fast-track-dispatch-blocker.txt").read_bytes()
        try:
            bundle(root, root / "fast-track-dispatch-blocker.txt", full=True, raw=True)
        except ValueError:
            pass
        else:
            raise AssertionError("a diagnostic input must not be overwritten")
        assert (root / "fast-track-dispatch-blocker.txt").read_bytes() == original
        try:
            bundle(root, full, full=True, raw=True, coverage=root / "absent.json")
        except OSError:
            pass
        else:
            raise AssertionError("missing coverage must fail before replacing a bundle")
        assert sha256(full) == sha256(repeated)

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
    parser.add_argument(
        "--coverage",
        type=Path,
        help=(
            "optional rmcp01-dispatch-coverage.json from the global local-product scan"
        ),
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

    default_name = (
        "fast-track-run-full.zip" if args.full else "fast-track-run-compact.zip"
    )
    output = (
        args.output.expanduser().resolve()
        if args.output is not None
        else source / default_name
    )

    coverage = (
        args.coverage.expanduser().resolve() if args.coverage is not None else None
    )
    if coverage is not None and not coverage.is_file():
        parser.error(f"coverage report does not exist: {coverage}")

    try:
        total, included, included_bytes = bundle(
            source,
            output,
            full=args.full,
            raw=args.raw,
            coverage=coverage,
        )
    except (OSError, ValueError) as exc:
        print(f"error: {exc}")
        return 2

    total_bytes = sum(path.stat().st_size for path in discover(source))
    ratio = (included_bytes / total_bytes * 100.0) if total_bytes else 0.0
    print(f"created: {output}")
    print(f"source diagnostics : {total} files / {total_bytes} bytes")
    print(
        f"report selection   : {included} files / {included_bytes} bytes ({ratio:.1f}%)"
    )
    print("archive contents   : fast-track-report.txt + manifest.txt")
    if args.raw:
        print("raw originals      : included under raw/")
    if coverage is not None:
        print("static coverage    : rmcp01-dispatch-coverage.json")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
