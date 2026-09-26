#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR

readonly DEPS_DIR="${MKW_M3_DEPS_DIR:-$ROOT_DIR/.deps/m3}"
readonly MESA_DIR="${MKW_M3_MESA_ROOT:-$DEPS_DIR/mesa-switch}"
readonly DAWN_DIR="${MKW_M3_DAWN_ROOT:-$DEPS_DIR/dawn-switch}"
readonly DAWN_BUILD_DIR="${MKW_M3_HLE_FIFO_BUILD_ROOT:-$DEPS_DIR/dawn-switch-hle-fifo-aurora-build}"
readonly WII_DIR="$ROOT_DIR/third_party/WiiCompiled"
readonly MESA_IMAGE="${MKW_M3_MESA_IMAGE:-wiicompiled-m3-mesa-b297e230-v3}"
readonly OUTPUT="$ROOT_DIR/WiiCompiled-Switch-local-rendered-fast-track.nro"
readonly SHARD_ROOT="$ROOT_DIR/local-product/generated/build_shards"
readonly COMMON="$SHARD_ROOT/base_common"
readonly SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
readonly DISPATCH="$SHARD_ROOT/base_dispatch"
readonly JOBS="${MKW_JOBS:-4}"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need docker
need python3

for required in \
    "$ROOT_DIR/local-product/generated/data_sections_init.cpp" \
    "$ROOT_DIR/local-product/generated/data_sections_init_blobs.S" \
    "$ROOT_DIR/local-product/generated/RuntimeConfig.h" \
    "$ROOT_DIR/local-product/generated/base_translation_output.json"; do
    if [[ ! -f "$required" ]]; then
        echo "error: missing $required; prepare the local RMCP01 product first" >&2
        exit 2
    fi
done

if [[ ! -d "$COMMON" ]]; then
    echo "error: missing $COMMON; run scripts/prepare-local-function-shards.sh first" >&2
    exit 2
fi
if [[ ! -d "$DISPATCH" ]] || ! compgen -G "$DISPATCH/*.cpp" >/dev/null; then
    echo "error: missing generated base dispatch under $DISPATCH" >&2
    exit 2
fi

normalize_paths=("$COMMON")
if [[ -d "$SENSITIVE" ]]; then
    normalize_paths+=("$SENSITIVE")
fi
echo "[1/4] Normalizing translated shards for devkitA64 GCC..."
python3 "$ROOT_DIR/scripts/normalize-gcc-statefree-returns.py" "${normalize_paths[@]}"

echo "[2/4] Preparing the hardware-proven Dawn/Aurora/NVK build environment..."
MKW_JOBS="$JOBS" bash "$ROOT_DIR/scripts/build-m3-hle-fifo-aurora-probe.sh"

DOCKER_SECURITY_ARGS=()
if command -v getenforce >/dev/null 2>&1 && [[ "$(getenforce)" == "Enforcing" ]]; then
    DOCKER_SECURITY_ARGS+=(--security-opt label=disable)
fi

echo "[3/4] Reconfiguring the proven build tree for the local RMCP01 rendered target..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -e MKW_M3_JOBS="$JOBS" \
    -v "$MESA_DIR:/mesa:ro" \
    -v "$DAWN_DIR:/dawn" \
    -v "$DAWN_BUILD_DIR:/build" \
    -v "$WII_DIR:/wiicompiled:ro" \
    -v "$ROOT_DIR/m3-hle-fifo-probe:/probe:ro" \
    -v "$ROOT_DIR:/repo:ro" \
    -v "$ROOT_DIR:$ROOT_DIR:ro" \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        export DEVKITPRO=/opt/devkitpro
        cmake -S /dawn -B /build -DM3_BUILD_RENDERED_FAST_TRACK=ON
        cmake --build /build --target mkw_switch_rendered_fast_track_nro -j"$MKW_M3_JOBS"
    '

echo "[4/4] Collecting local game-containing NRO..."
built_nro="$(find "$DAWN_BUILD_DIR" -type f -name 'WiiCompiled-Switch-local-rendered-fast-track.nro' -print -quit)"
if [[ -z "$built_nro" || ! -f "$built_nro" ]]; then
    echo "error: rendered fast-track NRO was not produced" >&2
    exit 1
fi
cp -f "$built_nro" "$OUTPUT"

echo
echo "RMCP01 rendered fast-track ready:"
echo "  $OUTPUT"
echo
echo "This NRO contains locally generated game-derived code. Do not upload or commit it."
echo
echo "Current RMCP01 resource gate:"
echo "  Copy your own extracted PAL RMCP01 DATA directory to:"
echo "  /switch/WiiCompiled-Switch/DATA"
echo "  Required now: DATA/sys/boot.bin, DATA/sys/fst.bin, and DATA/files/"
echo
echo "Launch it through hbmenu title override/application mode."
echo
echo "After the run, return:"
echo "  /switch/WiiCompiled-Switch/dvd-fst-status.txt"
echo "  /switch/WiiCompiled-Switch/dvd-read-status.txt"
echo "  /switch/WiiCompiled-Switch/rendered-fast-track-graphics.txt"
echo "  /switch/WiiCompiled-Switch/fast-track-progress.txt"
echo "  /switch/WiiCompiled-Switch/fast-track-heartbeat.txt"
echo "  /switch/WiiCompiled-Switch/fast-track-thread-events.txt"
echo "  /switch/WiiCompiled-Switch/fast-track-post-video-trace.txt"
echo "  /switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt"
echo "  /switch/WiiCompiled-Switch/fast-track-exception.txt"
echo
echo "Optional: after copying the SD diagnostics to your PC, bundle them into one"
echo "compact upload artifact with:"
echo "  python3 scripts/package-fast-track-run.py /path/to/copied/WiiCompiled-Switch"
echo "Use --full when scheduler/thread/liveness traces are needed."
