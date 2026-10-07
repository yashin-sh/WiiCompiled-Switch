#!/usr/bin/env python3
"""Instrument only a build mirror of the audited Aurora FIFO drain."""

import argparse
import hashlib
import importlib.util
import subprocess
from pathlib import Path

WII_PIN = "a135beb201042b20f390c6695ca6b26768820fb4"
DAWN_PIN = "77029ea85250c9bdddfc2f88034afb6b5356a031"


def prepare(source: Path, destination: Path, dawn: Path) -> None:
    for checkout, expected in ((source.parent, WII_PIN), (dawn, DAWN_PIN)):
        actual = subprocess.check_output(
            ["git", "-C", str(checkout), "rev-parse", "HEAD"], text=True
        ).strip()
        if actual != expected:
            raise ValueError(f"replay dependency pin mismatch: {checkout}: {actual}")
    subprocess.run(
        [
            "git",
            "-C",
            str(dawn),
            "diff",
            "--exit-code",
            "HEAD",
            "--",
            "src",
            "include",
            ":(exclude)src/dawn/switch",
            ":(exclude)src/dawn/native/CMakeLists.txt",
        ],
        check=True,
        capture_output=True,
    )
    subprocess.run(
        [
            "git",
            "-C",
            str(source.parent),
            "diff",
            "--exit-code",
            "HEAD",
            "--",
            "aurora-main/lib",
            "aurora-main/include",
            ":(exclude)aurora-main/include/dolphin/gx/GXGeometry.h",
        ],
        check=True,
        capture_output=True,
    )
    fifo = source / "lib/gx/fifo.cpp"
    # The common drain is the only instrumentation site in this prototype.
    digest = hashlib.sha256(fifo.read_bytes()).hexdigest()
    if digest != FIFO_SHA256:
        raise ValueError("Aurora FIFO drain changed; re-audit required")
    spec = importlib.util.spec_from_file_location(
        "checked_fifo", Path(__file__).with_name("prepare-checked-aurora-fifo.py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.prepare(source, destination)
    # Use the same source pin on a clean checkout and on the existing local
    # Switch integration tree. Only the known C-linkage overload is removed.
    geometry = subprocess.check_output(
        [
            "git",
            "-C",
            str(source.parent),
            "show",
            f"{WII_PIN}:aurora-main/include/dolphin/gx/GXGeometry.h",
        ],
        text=True,
    )
    overload = (
        "static inline void GXSetArray(GXAttr attr, const void* data, u32 size, u8 stride) {\n"
        "  GXSetArray(attr, data, size, stride, false);\n}\n"
    )
    if geometry.count(overload) != 1:
        raise ValueError("pinned geometry overload changed")
    for original in sorted((source / "include").rglob("*")):
        if not original.is_file():
            continue
        target = destination / original.relative_to(source)
        content = (
            geometry.replace(overload, "").encode()
            if original == source / "include/dolphin/gx/GXGeometry.h"
            else original.read_bytes()
        )
        if not target.exists() or target.read_bytes() != content:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
    text = fifo.read_text()
    needle = "  process(detail::sBufferData, detail::sBufferSize, true);"
    if text.count(needle) != 1:
        raise ValueError("FIFO drain instrumentation is ambiguous")
    text = text.replace(
        needle,
        "  mkw_replay_capture_drain(detail::sBufferData, detail::sBufferSize);\n"
        + needle,
    )
    text = (
        'extern "C" void mkw_replay_capture_drain(const unsigned char*, unsigned int);\n'
        + text
    )
    target = destination / "lib/gx/fifo.cpp"
    if target.read_text() != text:
        target.write_text(text)


FIFO_SHA256 = "62a5355cf5741b586eea18f74fbcdd620e56dcdae40756a129176007665311c8"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("dawn", type=Path)
    args = parser.parse_args()
    prepare(args.source.resolve(), args.destination.resolve(), args.dawn.resolve())
