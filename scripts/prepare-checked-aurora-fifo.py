#!/usr/bin/env python3
"""Prepare a build-only Aurora source mirror with bounded display-list writes."""

import argparse
import hashlib
import shutil
from pathlib import Path

FIFO_SHA256 = "6070d40ebda0c909cc5356eaa241e42f2980069c294b41bcf8d1f4c74c5dd17c"


def checked_fifo(text: str) -> str:
    if hashlib.sha256(text.encode()).hexdigest() != FIFO_SHA256:
        raise ValueError("pinned Aurora FIFO header changed; re-audit required")
    text = text.replace(
        "void init();",
        'extern "C" [[noreturn]] void mkw_switch_gx_display_list_overflow(uint32_t length);\n\nvoid init();',
    )
    text = text.replace(
        "  else if (detail::sDlWritePos + length <= detail::sDlSize) {\n"
        "    std::memcpy(detail::sDlBuffer + detail::sDlWritePos, data, length);",
        "  else {\n"
        "    if (detail::sDlWritePos > detail::sDlSize || length > detail::sDlSize - detail::sDlWritePos) {\n"
        "      mkw_switch_gx_display_list_overflow(length);\n"
        "    }\n"
        "    std::memmove(detail::sDlBuffer + detail::sDlWritePos, data, length);",
    )
    text = text.replace(
        "  else if (detail::sDlWritePos < detail::sDlSize) {\n"
        "    detail::sDlBuffer[detail::sDlWritePos++] = val;",
        "  else {\n"
        "    if (detail::sDlWritePos >= detail::sDlSize) {\n"
        "      mkw_switch_gx_display_list_overflow(1);\n"
        "    }\n"
        "    detail::sDlBuffer[detail::sDlWritePos++] = val;",
    )
    return text


def prepare(source: Path, destination: Path) -> None:
    source = source.resolve(strict=True)
    destination = destination.resolve()
    if (
        destination == source
        or source in destination.parents
        or destination in source.parents
    ):
        raise ValueError("the build mirror must be outside the original Aurora tree")
    header = checked_fifo((source / "lib/gx/fifo.hpp").read_text())
    for original in sorted((source / "lib").rglob("*")):
        if not original.is_file():
            continue
        target = destination / original.relative_to(source)
        content = (
            header.encode()
            if original == source / "lib/gx/fifo.hpp"
            else original.read_bytes()
        )
        if not target.exists() or target.read_bytes() != content:
            target.parent.mkdir(parents=True, exist_ok=True)
            if original == source / "lib/gx/fifo.hpp":
                target.write_bytes(content)
            else:
                shutil.copy2(original, target)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
