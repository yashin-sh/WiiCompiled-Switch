#!/usr/bin/env python3
"""Prepare checked FIFO writes, scalars and copy textures in a build mirror."""

import argparse
import hashlib
import shutil
from pathlib import Path

FIFO_SHA256 = "6070d40ebda0c909cc5356eaa241e42f2980069c294b41bcf8d1f4c74c5dd17c"

VERT_SHA256 = "0f1f91b132507547a65a34c6acc7b4425748ca5ea5542360270e8908ea6eaecc"
COMMON_SHA256 = "8569fa8facd4f5062730e5011db87b4591129225c72956991fc4b7592cdec6e1"


def checked_common(text: str) -> str:
    if hashlib.sha256(text.encode()).hexdigest() != COMMON_SHA256:
        raise ValueError("pinned Aurora copy renderer changed; re-audit required")
    needle = (
        '      .label = "GX Copy Source Snapshot",\n'
        "      .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,"
    )
    if text.count(needle) != 1:
        raise ValueError("copy source snapshot descriptor is ambiguous")
    # The snapshot is both a copy destination and an exact-copy source;
    # shader-sampled paths also retain their existing TextureBinding usage.
    return text.replace(needle, needle[:-1] + " | wgpu::TextureUsage::CopySrc,")


def checked_vert(text: str) -> str:
    if hashlib.sha256(text.encode()).hexdigest() != VERT_SHA256:
        raise ValueError("pinned Aurora vertex writer changed; re-audit required")
    # Float-kind consumers use only f. Eager signed/unsigned casts are unused
    # and undefined for coordinates outside the corresponding integer range.
    return text.replace(
        "Scalar{.kind = ScalarKind::Float, .f = value, .s = static_cast<int32_t>(value), .u = static_cast<uint32_t>(value)}",
        "Scalar{.kind = ScalarKind::Float, .f = value, .s = 0, .u = 0}",
    )


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
    replacements = {
        source / "lib/gfx/common.cpp": checked_common(
            (source / "lib/gfx/common.cpp").read_text()
        ).encode(),
        source / "lib/gx/fifo.hpp": checked_fifo(
            (source / "lib/gx/fifo.hpp").read_text()
        ).encode(),
        source / "lib/dolphin/gx/GXVert.cpp": checked_vert(
            (source / "lib/dolphin/gx/GXVert.cpp").read_text()
        ).encode(),
    }
    for original in sorted((source / "lib").rglob("*")):
        if not original.is_file():
            continue
        target = destination / original.relative_to(source)
        content = replacements.get(original, original.read_bytes())
        if not target.exists() or target.read_bytes() != content:
            target.parent.mkdir(parents=True, exist_ok=True)
            if original in replacements:
                target.write_bytes(content)
            else:
                shutil.copy2(original, target)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
