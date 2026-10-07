#!/usr/bin/env python3
"""Instrument only a build mirror of the audited Aurora FIFO drain."""

import argparse
import hashlib
import importlib.util
import subprocess
from pathlib import Path

WII_PIN = "a135beb201042b20f390c6695ca6b26768820fb4"
DAWN_PIN = "77029ea85250c9bdddfc2f88034afb6b5356a031"


def git_args(checkout: Path) -> list[str]:
    git_file = checkout / ".git"
    if git_file.is_file():
        git_dir = checkout / git_file.read_text().strip().removeprefix("gitdir: ")
        if not git_dir.exists():
            # Docker mounts the same checkout at /wiicompiled and /repo/...;
            # relative submodule metadata resolves only from the latter.
            candidate = Path(__file__).resolve().parents[1] / "third_party/WiiCompiled"
            if not candidate.samefile(checkout):
                raise ValueError("cannot resolve capture submodule Git metadata")
            checkout = candidate
    return ["git", "-c", f"safe.directory={checkout}", "-C", str(checkout)]


def prepare(source: Path, destination: Path, dawn: Path) -> None:
    for checkout, expected in ((source.parent, WII_PIN), (dawn, DAWN_PIN)):
        actual = subprocess.check_output(
            [*git_args(checkout), "rev-parse", "HEAD"], text=True
        ).strip()
        if actual != expected:
            raise ValueError(f"replay dependency pin mismatch: {checkout}: {actual}")
    subprocess.run(
        [
            *git_args(dawn),
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
            *git_args(source.parent),
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
    # Instrument pinned consumption sites, including direct display lists and EFB copies.
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
            *git_args(source.parent),
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
        'extern "C" void mkw_replay_capture_drain(const unsigned char*, unsigned int) noexcept;\n'
        + text
    )
    target = destination / "lib/gx/fifo.cpp"
    if target.read_text() != text:
        target.write_text(text)

    hooks = {
        "lib/gx/command_processor.cpp": [
            (
                "  // This entry point bypasses process(), so it owns the renderer lock itself.",
                "  mkw_replay_capture_raw_draw(static_cast<unsigned>(prim), static_cast<unsigned>(fmt), vertices, vtxCount, vertexBytes);\n  // This entry point bypasses process(), so it owns the renderer lock itself.",
            ),
        ],
        "lib/dolphin/gx/GXManage.cpp": [
            (
                "GXFifoObj* GXInit(void* base, u32 size) {",
                "GXFifoObj* GXInit(void* base, u32 size) {\n  mkw_replay_capture_init();",
            )
        ],
        "lib/dolphin/gx/GXDispList.cpp": [
            (
                "  aurora::gx::fifo::process(static_cast<const u8*>(data), nbytes, true);",
                "  mkw_replay_capture_drain(static_cast<const u8*>(data), nbytes);\n  aurora::gx::fifo::process(static_cast<const u8*>(data), nbytes, true);",
            ),
            (
                "  aurora::gx::fifo::process(static_cast<const u8*>(data), nbytes, false);",
                '  mkw_replay_capture_unsupported("little-endian display list");\n  aurora::gx::fifo::process(static_cast<const u8*>(data), nbytes, false);',
            ),
        ],
        "lib/dolphin/gx/GXFrameBuffer.cpp": [],
        "lib/dolphin/gx/GXAurora.cpp": [
            (
                "  g_gxState.viewportPolicy = policy;",
                "  mkw_replay_capture_mapping(policy);\n  g_gxState.viewportPolicy = policy;",
            ),
            (
                "void GXCreateFrameBuffer(u32 width, u32 height) {",
                'void GXCreateFrameBuffer(u32 width, u32 height) {\n  mkw_replay_capture_unsupported("offscreen framebuffer");',
            ),
            (
                "void GXRestoreFrameBuffer() {",
                'void GXRestoreFrameBuffer() {\n  mkw_replay_capture_unsupported("offscreen framebuffer restore");',
            ),
        ],
    }
    for relative, substitutions in hooks.items():
        original = (source / relative).read_text()
        if hashlib.sha256(original.encode()).hexdigest() != HOOK_SHA256[relative]:
            raise ValueError(f"capture hook source changed: {relative}")
        if relative.endswith("GXFrameBuffer.cpp"):
            for name, texture in (("GXCopyDisp", "false"), ("GXCopyTex", "true")):
                start = original.index(f"void {name}(void* dest, GXBool clear) {{")
                marker = "    aurora::gx::fifo::drain();\n  }"
                end = original.index(marker, start) + len(marker)
                original = (
                    original[:end]
                    + f"\n  mkw_replay_capture_copy({texture}, dest, clear != GX_FALSE);"
                    + original[end:]
                )
        for needle, replacement in substitutions:
            if original.count(needle) != 1:
                raise ValueError(f"ambiguous capture hook: {relative}: {needle}")
            original = original.replace(needle, replacement)
        declarations = (
            'extern "C" void mkw_replay_capture_init() noexcept;\n'
            'extern "C" void mkw_replay_capture_raw_draw(unsigned, unsigned, const unsigned char*, unsigned short, unsigned) noexcept;\n'
            'extern "C" void mkw_replay_capture_drain(const unsigned char*, unsigned int) noexcept;\n'
            'extern "C" void mkw_replay_capture_copy(bool, void*, bool) noexcept;\n'
            'extern "C" void mkw_replay_capture_mapping(unsigned) noexcept;\n'
            'extern "C" void mkw_replay_capture_unsupported(const char*) noexcept;\n'
        )
        target = destination / relative
        target.write_text(declarations + original)


HOOK_SHA256 = {
    "lib/gx/command_processor.cpp": "01fb6258987b4203a5add858c8b4fe56ec1a47cf8c32b85eaa7994da2e1bc038",
    "lib/dolphin/gx/GXManage.cpp": "3d3f40599683b5cb6e811e4f46b5ac71cf6fbb632f3bf39fab830c95c38e6fbb",
    "lib/dolphin/gx/GXDispList.cpp": "afd8d00985373dddd732867445a03d097e6fc3bac3f8846d3e1ab79506d8174e",
    "lib/dolphin/gx/GXFrameBuffer.cpp": "7a080e1aa4a39ae5f99bcdab34521852f296dbe9c69955c9abd4c2c8283c676e",
    "lib/dolphin/gx/GXAurora.cpp": "020a534a52eea4d7bc5984185b839a7c6e9fe3087c9c153af34131920560a48b",
}

FIFO_SHA256 = "62a5355cf5741b586eea18f74fbcdd620e56dcdae40756a129176007665311c8"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("dawn", type=Path)
    args = parser.parse_args()
    prepare(args.source.resolve(), args.destination.resolve(), args.dawn.resolve())
