#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly REF_DIR="$ROOT_DIR/third_party/DriftDroid"
readonly REF_PIN="8b33d31016a8b8735fe35b4fb633c491d33774c6"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need git

git -C "$ROOT_DIR" submodule update --init --recursive third_party/DriftDroid

if [[ ! -d "$REF_DIR/.git" && ! -f "$REF_DIR/.git" ]]; then
    echo "error: DriftDroid submodule was not initialized: $REF_DIR" >&2
    exit 2
fi

actual_pin="$(git -C "$REF_DIR" rev-parse HEAD)"
if [[ "$actual_pin" != "$REF_PIN" ]]; then
    echo "error: DriftDroid pin mismatch: expected $REF_PIN, got $actual_pin" >&2
    exit 2
fi

require_marker() {
    local relative_path="$1"
    local marker="$2"
    local full_path="$REF_DIR/$relative_path"

    if [[ ! -f "$full_path" ]]; then
        echo "error: missing pinned reference file: $relative_path" >&2
        exit 2
    fi
    if ! grep -Fq -- "$marker" "$full_path"; then
        echo "error: pinned reference lost required marker '$marker' in $relative_path" >&2
        exit 2
    fi
}

# Performance/correctness contracts that make this reference materially different
# from the repository's current Dawn/Vulkan/NVK bring-up path.
require_marker "aurora-main/cmake/AuroraDawnProvider.cmake" \
    'option(AURORA_SWITCH_DEKO3D_DEBUG "Link the validating debug build of deko3d" OFF)'
require_marker "aurora-main/cmake/AuroraDawnProvider.cmake" \
    'set(_aurora_deko3d_lib deko3d)'
require_marker "runtime/CMakeLists.txt" \
    'option(MKW_SWITCH_SECTION_ORDERING "Order hot functions first in the Switch NRO" ON)'
require_marker "runtime/CMakeLists.txt" \
    'set(MKW_PGO "" CACHE STRING "PGO pass (Switch and Android): empty, generate or use")'
require_marker "runtime/src/settings_overlay.cpp" \
    'bool g_threadedGx = RuntimeConfigFile::ThreadedGx(true);'
require_marker "runtime/include/runtime_config.h" \
    'config.overlapFrameEncode = FindConfigValue<bool>(document, "video", "overlap_frame_encode");'

cat <<EOF
Pinned native Switch reference is ready.
  source : third_party/DriftDroid
  commit : $REF_PIN

Verified contracts:
  - release Deko3D is the default Switch link mode
  - Deko3D validation remains opt-in
  - hot-section ordering is enabled by default
  - PGO generate/use support is present
  - threaded GX defaults on for Switch
  - frame-encode overlap is configurable

This command only verifies public source. It does not create, copy or publish game data.
See docs/NATIVE_DEKO3D_SWITCH_PATH_2026-10-12.md for the hardware-gated migration plan.
EOF
