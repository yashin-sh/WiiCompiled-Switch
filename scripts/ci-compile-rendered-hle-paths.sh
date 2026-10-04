#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR

CXX_TOOL="${1:-${CXX_TOOL:-}}"
if [[ -z "$CXX_TOOL" ]]; then
    if [[ -z "${DEVKITPRO:-}" ]]; then
        echo "error: pass the devkitA64 g++ path or set DEVKITPRO" >&2
        exit 2
    fi
    CXX_TOOL="$(find "$DEVKITPRO" -type f -name 'aarch64-none-elf-g++' -print -quit)"
fi
if [[ -z "$CXX_TOOL" || ! -x "$CXX_TOOL" ]]; then
    echo "error: devkitA64 g++ not found: $CXX_TOOL" >&2
    exit 2
fi

readonly WII_DIR="$ROOT_DIR/third_party/WiiCompiled"
readonly RUNTIME_DIR="$WII_DIR/runtime"
readonly AURORA_DIR="$WII_DIR/aurora-main"
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"
readonly WII_RENDERED_PATCH="$ROOT_DIR/patches/wiicompiled/m3-wiicompiled-switch-build.patch"

if [[ "$(git -C "$WII_DIR" rev-parse HEAD)" != "$WII_PIN" ]]; then
    echo "error: WiiCompiled pin mismatch for rendered syntax gate" >&2
    exit 2
fi

patched_paths=(
    aurora-main/include/dolphin/gx/GXGeometry.h
    runtime/include/abi_bridge.h
    runtime/include/gx_guest_write.h
    runtime/include/runtime_config.h
    runtime/include/runtime_log.h
    runtime/include/system_bridge.h
    runtime/src/hle/gx/gx_internal.h
    runtime/src/hle/gx/gx_stream_common.h
    runtime/src/hle/gx/gx_dl.cpp
)

patch_applied=0
restore_wiicompiled() {
    local exit_status=$?
    trap - EXIT
    if ((patch_applied != 0)); then
        if ! git -C "$WII_DIR" apply --reverse "$WII_RENDERED_PATCH"; then
            echo "error: could not undo the rendered syntax gate's own patch; existing edits were preserved" >&2
            if ((exit_status == 0)); then
                exit_status=2
            fi
        fi
    fi
    exit "$exit_status"
}
trap restore_wiicompiled EXIT

# Reuse the configured patch without rewriting headers or removing local
# changes. A clean checkout receives a temporary patch owned by this gate.
if git -C "$WII_DIR" apply --reverse --check "$WII_RENDERED_PATCH" >/dev/null 2>&1; then
    echo "Reusing the existing rendered WiiCompiled patch"
else
    if ! git -C "$WII_DIR" diff --quiet -- "${patched_paths[@]}" ||
       ! git -C "$WII_DIR" diff --cached --quiet -- "${patched_paths[@]}"; then
        echo "error: rendered header paths have local changes; refusing to overwrite them" >&2
        exit 2
    fi
    if ! git -C "$WII_DIR" apply --check "$WII_RENDERED_PATCH"; then
        echo "error: rendered WiiCompiled patch no longer applies to $WII_PIN" >&2
        exit 2
    fi
    git -C "$WII_DIR" apply "$WII_RENDERED_PATCH"
    patch_applied=1
fi

for required in \
    "$RUNTIME_DIR/include/host_context.h" \
    "$RUNTIME_DIR/src/hle/gx/gx_internal.h" \
    "$AURORA_DIR/include/dolphin/gx.h" \
    "$ROOT_DIR/local-rendered-fast-track/seams/hle_stubs.h" \
    "$ROOT_DIR/ci-rendered-compile-seams/aurora_events.h" \
    "$WII_RENDERED_PATCH"; do
    if [[ ! -f "$required" ]]; then
        echo "error: missing rendered compile dependency: $required" >&2
        exit 2
    fi
done

# PADRead is an input bridge on the same executed path, with no rendered macro.
mapfile -t rendered_sources < <(
    { printf '%s\n' "$ROOT_DIR/source/pad_read_hle_bridge.cpp"; grep -l 'MKW_LOCAL_RENDERED_FAST_TRACK' "$ROOT_DIR"/source/*_hle_bridge.cpp; } | sort -u
)

if (( ${#rendered_sources[@]} == 0 )); then
    echo "error: no rendered HLE sources discovered" >&2
    exit 2
fi

common_flags=(
    -std=gnu++20
    -fsyntax-only
    -Wall
    -Wextra
    -fno-rtti
    -fno-fast-math
    -ffp-contract=off
    -fno-tree-slp-vectorize
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp"
    -DAURORA
    -DAURORA_ENABLE_GX
    -DTARGET_PC
    -DWEBGPU_DAWN
    -D__SWITCH__
    -DNX
    -DVK_USE_PLATFORM_VI_NN
    -DMKW_PLATFORM_SWITCH=1
    -DMKW_LOCAL_FUNCTION_EXECUTION=1
    -DMKW_LOCAL_FAST_TRACK=1
    -DMKW_LOCAL_RENDERED_FAST_TRACK=1
    -DMKW_ENABLE_DATA_INIT_HANDOFF=1
    -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1
    -I"$ROOT_DIR/ci-rendered-compile-seams"
    -I"$ROOT_DIR/local-rendered-fast-track/seams"
    -I"$ROOT_DIR/m3-aurora-gx-probe/compat"
    -I"$ROOT_DIR/m3-aurora-gx-probe/source"
    -I"$ROOT_DIR/include"
    -I"$RUNTIME_DIR/include"
    -I"$RUNTIME_DIR/src"
    -I"$RUNTIME_DIR/src/hle/gx"
    -I"$RUNTIME_DIR/third_party/toml11"
    -I"$AURORA_DIR/include"
    -I"$AURORA_DIR/lib"
)

# Resolve SDK headers even when the compiler is supplied as an explicit path.
SDK_ROOT="${DEVKITPRO:-$(cd "$(dirname "$CXX_TOOL")/../.." && pwd)}"
readonly SDK_ROOT
if [[ ! -d "$SDK_ROOT/libnx/include" ]]; then
    echo "error: libnx headers missing: $SDK_ROOT/libnx/include" >&2
    exit 2
fi
common_flags+=( -I"$SDK_ROOT/libnx/include" )

echo "Rendered HLE syntax gate: ${#rendered_sources[@]} source files"
for source in "${rendered_sources[@]}"; do
    rel="${source#"$ROOT_DIR/"}"
    echo "  CXX $rel"
    "$CXX_TOOL" "${common_flags[@]}" "$source"
done

echo "  CXX source/pad_read_hle_bridge.cpp (synthetic mode without desktop defines)"
"$CXX_TOOL" -std=gnu++20 -fsyntax-only -Wall -Wextra -fno-rtti \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -DMKW_SYNTHETIC_EXECUTION=1 -D__SWITCH__ -DMKW_PLATFORM_SWITCH=1 -I"$ROOT_DIR/include" -I"$RUNTIME_DIR/include" \
    "$ROOT_DIR/source/pad_read_hle_bridge.cpp"

echo "  CXX source/horizon_runtime_services.cpp (actual libnx input backend)"
"$CXX_TOOL" "${common_flags[@]}" "$ROOT_DIR/source/horizon_runtime_services.cpp"

echo "  CXX source/fast_track_crash_diagnostics.cpp (Discovery mode)"
"$CXX_TOOL" "${common_flags[@]}"     -DMKW_DISCOVERY_SCAN_MODE=1     "$ROOT_DIR/source/fast_track_crash_diagnostics.cpp"

echo "PASS: all rendered HLE branches and Discovery diagnostics compile against pinned WiiCompiled/Aurora headers"
