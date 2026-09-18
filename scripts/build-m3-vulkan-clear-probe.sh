#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly MESA_PIN="b297e230ef88c6c88df2561becf864f979f494a6"
readonly MESA_REPO="https://github.com/danfromtico/mesa-switch.git"
readonly DEPS_DIR="${MKW_M3_DEPS_DIR:-$ROOT_DIR/.deps/m3}"
readonly MESA_DIR="${MKW_M3_MESA_ROOT:-$DEPS_DIR/mesa-switch}"
readonly MESA_IMAGE="${MKW_M3_MESA_IMAGE:-devkitpro-mesa-rust}"
readonly JOBS="${MKW_JOBS:-4}"
readonly VULKAN_ARCHIVE="$MESA_DIR/builddir-switch/src/nouveau/vulkan/libvulkan.a"
readonly PROBE_DIR="$ROOT_DIR/m3-graphics-probe"
readonly OUTPUT="$PROBE_DIR/WiiCompiled-Switch-m3-vulkan-clear-probe.nro"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need git
need docker

mkdir -p "$DEPS_DIR"

if [[ ! -d "$MESA_DIR/.git" ]]; then
    echo "[1/4] Cloning pinned mesa-switch dependency..."
    git clone --filter=blob:none "$MESA_REPO" "$MESA_DIR"
else
    echo "[1/4] Reusing mesa-switch checkout..."
fi

git -C "$MESA_DIR" fetch --quiet origin "$MESA_PIN"
git -C "$MESA_DIR" checkout --quiet --detach "$MESA_PIN"

actual_pin="$(git -C "$MESA_DIR" rev-parse HEAD)"
if [[ "$actual_pin" != "$MESA_PIN" ]]; then
    echo "error: mesa-switch pin mismatch: $actual_pin" >&2
    exit 1
fi
echo "      mesa-switch: $actual_pin"

if [[ ! -f "$VULKAN_ARCHIVE" || "${MKW_M3_FORCE_MESA_REBUILD:-0}" == "1" ]]; then
    echo "[2/4] Building loaderless NVK/Mesa for Switch..."
    (
        cd "$MESA_DIR"
        ./build-switch.sh
    )
else
    echo "[2/4] Reusing existing loaderless NVK archive."
fi

if ! docker image inspect "$MESA_IMAGE" >/dev/null 2>&1; then
    echo "error: expected Mesa build image '$MESA_IMAGE' is missing" >&2
    exit 1
fi
if [[ ! -f "$VULKAN_ARCHIVE" ]]; then
    echo "error: Mesa build did not produce $VULKAN_ARCHIVE" >&2
    exit 1
fi

echo "[3/4] Building isolated M3 Vulkan clear probe..."
docker run --rm     -v "$MESA_DIR:/mesa:ro"     -v "$ROOT_DIR:/work"     -w /work/m3-graphics-probe     "$MESA_IMAGE"     bash -lc "export DEVKITPRO=/opt/devkitpro; make -j'$JOBS' MESA_SWITCH_ROOT=/mesa"

if [[ ! -f "$OUTPUT" ]]; then
    echo "error: probe NRO was not produced: $OUTPUT" >&2
    exit 1
fi

echo "[4/4] Probe ready."
echo "NRO: $OUTPUT"
echo
echo "Copy it to:"
echo "  /switch/WiiCompiled-Switch-m3-vulkan-clear-probe/WiiCompiled-Switch-m3-vulkan-clear-probe.nro"
echo
echo "Expected hardware result:"
echo "  - the screen continuously changes colour;"
echo "  - press + to exit;"
echo "  - send back /switch/WiiCompiled-Switch/m3-vulkan-clear-probe.txt."
