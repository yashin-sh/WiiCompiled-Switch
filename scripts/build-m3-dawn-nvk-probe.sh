#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly DAWN_REPO="https://github.com/google/dawn.git"
readonly DAWN_PIN="13abc3bc8ea2d3c2050f9e77a12d012108ceee24"
readonly DAWN_PACKAGE_TAG="v20260603.191052"
readonly MESA_PIN="b297e230ef88c6c88df2561becf864f979f494a6"
readonly DEPS_DIR="${MKW_M3_DEPS_DIR:-$ROOT_DIR/.deps/m3}"
readonly DAWN_DIR="${MKW_M3_DAWN_ROOT:-$DEPS_DIR/dawn}"
readonly DAWN_HOST_BUILD="$DEPS_DIR/dawn-host-build"
readonly DAWN_SWITCH_BUILD="$DEPS_DIR/dawn-switch-build"
readonly DAWN_INSTALL="$DEPS_DIR/dawn-switch-install"
readonly MESA_DIR="${MKW_M3_MESA_ROOT:-$DEPS_DIR/mesa-switch}"
readonly MESA_IMAGE="${MKW_M3_MESA_IMAGE:-wiicompiled-m3-mesa-b297e230-v3}"
readonly PATCHER="$ROOT_DIR/scripts/patch-m3-dawn-switch.py"
readonly PROBE_DIR="$ROOT_DIR/m3-dawn-probe"
readonly OUTPUT="$PROBE_DIR/WiiCompiled-Switch-m3-dawn-nvk-probe.nro"
readonly RUST_TARGET="aarch64-unknown-linux-gnu"
readonly JOBS="${MKW_JOBS:-4}"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need git
need docker
need python3

if [[ ! -f "$PATCHER" ]]; then
    echo "error: missing Dawn Switch patcher: $PATCHER" >&2
    exit 1
fi

echo "[1/7] Ensuring the hardware-proven mesa-switch/NVK toolchain..."
MKW_JOBS="$JOBS" bash "$ROOT_DIR/scripts/build-m3-vulkan-triangle-probe.sh" >/dev/null

actual_mesa="$(git -C "$MESA_DIR" rev-parse HEAD)"
if [[ "$actual_mesa" != "$MESA_PIN" ]]; then
    echo "error: mesa-switch pin mismatch: $actual_mesa" >&2
    exit 1
fi
if ! docker image inspect "$MESA_IMAGE" >/dev/null 2>&1; then
    echo "error: expected Mesa build image is missing: $MESA_IMAGE" >&2
    exit 1
fi

mkdir -p "$DEPS_DIR"
if [[ ! -d "$DAWN_DIR/.git" ]]; then
    echo "[2/7] Cloning Dawn..."
    git clone --filter=blob:none "$DAWN_REPO" "$DAWN_DIR"
else
    echo "[2/7] Reusing Dawn checkout..."
fi

git -C "$DAWN_DIR" fetch --quiet origin "$DAWN_PIN"
git -C "$DAWN_DIR" reset --hard --quiet "$DAWN_PIN"
git -C "$DAWN_DIR" checkout --quiet --detach "$DAWN_PIN"
actual_dawn="$(git -C "$DAWN_DIR" rev-parse HEAD)"
if [[ "$actual_dawn" != "$DAWN_PIN" ]]; then
    echo "error: Dawn source pin mismatch: $actual_dawn" >&2
    exit 1
fi

echo "      Dawn package provenance: $DAWN_PACKAGE_TAG -> $DAWN_PIN"
echo "[3/7] Applying narrow Horizon/loaderless-NVK Dawn patch..."
python3 "$PATCHER" "$DAWN_DIR"

DOCKER_SECURITY_ARGS=()
if command -v getenforce >/dev/null 2>&1 && [[ "$(getenforce)" == "Enforcing" ]]; then
    DOCKER_SECURITY_ARGS+=(--security-opt label=disable)
fi

echo "[4/7] Building host protoc required by Dawn cross-build..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -v "$ROOT_DIR:/work" \
    -w /work \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        cmake -S /work/.deps/m3/dawn -B /work/.deps/m3/dawn-host-build -G Ninja \
            -DDAWN_FETCH_DEPENDENCIES=ON \
            -DDAWN_BUILD_SAMPLES=OFF \
            -DDAWN_BUILD_TESTS=OFF \
            -DDAWN_BUILD_BENCHMARKS=OFF \
            -DDAWN_BUILD_NODE_BINDINGS=OFF \
            -DTINT_BUILD_TESTS=OFF \
            -DTINT_BUILD_CMD_TOOLS=OFF \
            -DTINT_BUILD_IR_BINARY=OFF \
            -DCMAKE_BUILD_TYPE=Release
        cmake --build /work/.deps/m3/dawn-host-build --target protoc -j"'"$JOBS"'"
    '

if [[ ! -x "$DAWN_HOST_BUILD/protoc" ]]; then
    echo "error: host protoc was not produced: $DAWN_HOST_BUILD/protoc" >&2
    exit 1
fi

echo "[5/7] Cross-building pinned Dawn/WebGPU for Horizon..."
rm -rf "$DAWN_SWITCH_BUILD" "$DAWN_INSTALL"
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -v "$ROOT_DIR:/work" \
    -w /work \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        export DEVKITPRO=/opt/devkitpro
        cmake -S /work/.deps/m3/dawn -B /work/.deps/m3/dawn-switch-build -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE=/work/m3-dawn-probe/switch-dawn-toolchain.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/work/.deps/m3/dawn-switch-install \
            -DWITH_PROTOC=/work/.deps/m3/dawn-host-build/protoc \
            -DDAWN_FETCH_DEPENDENCIES=ON \
            -DDAWN_ENABLE_INSTALL=ON \
            -DDAWN_BUILD_MONOLITHIC_LIBRARY=STATIC \
            -DDAWN_ENABLE_VULKAN=ON \
            -DDAWN_ENABLE_NULL=OFF \
            -DDAWN_ENABLE_D3D11=OFF \
            -DDAWN_ENABLE_D3D12=OFF \
            -DDAWN_ENABLE_METAL=OFF \
            -DDAWN_ENABLE_DESKTOP_GL=OFF \
            -DDAWN_ENABLE_OPENGLES=OFF \
            -DDAWN_ENABLE_SWIFTSHADER=OFF \
            -DDAWN_ENABLE_VULKAN_VALIDATION_LAYERS=OFF \
            -DDAWN_USE_X11=OFF \
            -DDAWN_USE_WAYLAND=OFF \
            -DDAWN_USE_GLFW=OFF \
            -DDAWN_SUPPORTS_GLFW_FOR_WINDOWING=OFF \
            -DDAWN_BUILD_SAMPLES=OFF \
            -DDAWN_BUILD_TESTS=OFF \
            -DDAWN_BUILD_BENCHMARKS=OFF \
            -DDAWN_BUILD_NODE_BINDINGS=OFF \
            -DTINT_BUILD_TESTS=OFF \
            -DTINT_BUILD_CMD_TOOLS=OFF \
            -DTINT_BUILD_IR_BINARY=OFF
        cmake --build /work/.deps/m3/dawn-switch-build --target webgpu_dawn -j"'"$JOBS"'"
        cmake --install /work/.deps/m3/dawn-switch-build
    '

if [[ ! -f "$DAWN_INSTALL/lib/libwebgpu_dawn.a" ]]; then
    echo "error: Dawn install did not produce libwebgpu_dawn.a" >&2
    exit 1
fi

echo "[6/7] Linking isolated Dawn/NVK offscreen probe..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -e MESA_SWITCH_RUST_TARGET="$RUST_TARGET" \
    -e MKW_M3_JOBS="$JOBS" \
    -v "$MESA_DIR:/mesa:ro" \
    -v "$DAWN_INSTALL:/dawn-install:ro" \
    -v "$ROOT_DIR:/work" \
    -w /work/m3-dawn-probe \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        export DEVKITPRO=/opt/devkitpro
        target_libdir="$(rustc --print target-libdir --target="$MESA_SWITCH_RUST_TARGET")"
        stems=(
            std panic_unwind object memchr addr2line gimli rustc_demangle
            std_detect hashbrown rustc_std_workspace_alloc miniz_oxide adler
            unwind cfg_if libc alloc rustc_std_workspace_core core compiler_builtins
        )
        shopt -s nullglob
        rust_libs=()
        for stem in "${stems[@]}"; do
            matches=("$target_libdir/lib${stem}-"*.rlib)
            if ((${#matches[@]} != 1)); then
                echo "error: expected exactly one Rust archive for $stem; found ${#matches[@]}" >&2
                exit 1
            fi
            rust_libs+=("${matches[0]}")
        done
        make clean >/dev/null 2>&1 || true
        make -j"$MKW_M3_JOBS" \
            MESA_SWITCH_ROOT=/mesa \
            DAWN_ROOT=/dawn-install \
            RUST_STD_LIBS="${rust_libs[*]}"
    '

if [[ ! -f "$OUTPUT" ]]; then
    echo "error: Dawn probe NRO was not produced: $OUTPUT" >&2
    exit 1
fi

echo "[7/7] Dawn/NVK probe ready."
echo "Dawn source: $DAWN_PIN"
echo "Mesa source: $MESA_PIN"
echo "NRO: $OUTPUT"
echo
echo "Copy it to:"
echo "  /switch/WiiCompiled-Switch-m3-dawn-nvk-probe/WiiCompiled-Switch-m3-dawn-nvk-probe.nro"
echo
echo "Expected report:"
echo "  /switch/WiiCompiled-Switch/m3-dawn-nvk-probe.txt"
echo
echo "PASS marker:"
echo "  PASS DAWN_WEBGPU_VULKAN_NVK_OFFSCREEN_TRIANGLE"
