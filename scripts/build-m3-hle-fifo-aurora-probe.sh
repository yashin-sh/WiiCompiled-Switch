#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR

readonly MESA_PIN="b297e230ef88c6c88df2561becf864f979f494a6"
readonly MESA_REPO="https://github.com/danfromtico/mesa-switch.git"
readonly DAWN_PIN="77029ea85250c9bdddfc2f88034afb6b5356a031"
readonly DAWN_REPO="https://github.com/danfromtico/dawn-switch.git"
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"

readonly DEPS_DIR="${MKW_M3_DEPS_DIR:-$ROOT_DIR/.deps/m3}"
readonly MESA_DIR="${MKW_M3_MESA_ROOT:-$DEPS_DIR/mesa-switch}"
readonly DAWN_DIR="${MKW_M3_DAWN_ROOT:-$DEPS_DIR/dawn-switch}"
readonly DAWN_BUILD_DIR="${MKW_M3_HLE_FIFO_BUILD_ROOT:-$DEPS_DIR/dawn-switch-hle-fifo-aurora-build}"
readonly WII_DIR="$ROOT_DIR/third_party/WiiCompiled"

readonly MESA_PATCH="$ROOT_DIR/patches/mesa-switch/m3-linux-build.patch"
readonly DAWN_PATCH="$ROOT_DIR/patches/dawn-switch/m3-static-nvk-link.patch"
readonly AURORA_TARGET_PATCH="$ROOT_DIR/patches/dawn-switch/m3-aurora-probe-target.patch"
readonly ABSEIL_DIR="$DAWN_DIR/third_party/abseil-cpp"
readonly ABSEIL_PATCH="$ROOT_DIR/patches/dawn-switch/m3-abseil-switch-newlib.patch"
readonly PROBE_DIR="$ROOT_DIR/m3-hle-fifo-probe"
readonly PROBE_SOURCE="$PROBE_DIR/source/main.cpp"
readonly OUTPUT_DIR="$PROBE_DIR"
readonly OUTPUT="$OUTPUT_DIR/WiiCompiled-Switch-m3-hle-fifo-aurora-probe.nro"

readonly MESA_IMAGE="${MKW_M3_MESA_IMAGE:-wiicompiled-m3-mesa-b297e230-v3}"
readonly RUST_TARGET="aarch64-unknown-linux-gnu"
readonly JOBS="${MKW_JOBS:-4}"
readonly VULKAN_ARCHIVE="$MESA_DIR/builddir-switch/src/nouveau/vulkan/libvulkan.a"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need git
need docker

if [[ ! -f "$MESA_PATCH" ]]; then
    echo "error: missing Mesa integration patch: $MESA_PATCH" >&2
    exit 1
fi
if [[ ! -f "$DAWN_PATCH" ]]; then
    echo "error: missing Dawn integration patch: $DAWN_PATCH" >&2
    exit 1
fi
if [[ ! -f "$AURORA_TARGET_PATCH" ]]; then
    echo "error: missing Dawn Aurora target patch: $AURORA_TARGET_PATCH" >&2
    exit 1
fi
if [[ ! -f "$ABSEIL_PATCH" ]]; then
    echo "error: missing Abseil Switch/newlib patch: $ABSEIL_PATCH" >&2
    exit 1
fi
if [[ ! -f "$PROBE_SOURCE" || ! -f "$PROBE_DIR/CMakeLists.txt" ]]; then
    echo "error: missing HleFifoWrite/Aurora probe sources under $PROBE_DIR" >&2
    exit 1
fi
if [[ ! -d "$WII_DIR/.git" && ! -f "$WII_DIR/.git" ]]; then
    echo "error: WiiCompiled submodule is not initialized: $WII_DIR" >&2
    echo "run: git submodule update --init --recursive" >&2
    exit 1
fi

DOCKER_SECURITY_ARGS=()
if command -v getenforce >/dev/null 2>&1; then
    selinux_mode="$(getenforce)"
    if [[ "$selinux_mode" == "Enforcing" ]]; then
        echo "SELinux Enforcing detected: using per-container label isolation."
        DOCKER_SECURITY_ARGS+=(--security-opt label=disable)
    fi
fi

mkdir -p "$DEPS_DIR" "$DAWN_BUILD_DIR" "$OUTPUT_DIR"

git -C "$WII_DIR" fetch --quiet origin "$WII_PIN"
git -C "$WII_DIR" checkout --quiet --detach "$WII_PIN"
actual_wii_pin="$(git -C "$WII_DIR" rev-parse HEAD)"
if [[ "$actual_wii_pin" != "$WII_PIN" ]]; then
    echo "error: WiiCompiled pin mismatch: $actual_wii_pin" >&2
    exit 1
fi
echo "      WiiCompiled: $actual_wii_pin"

if [[ ! -d "$MESA_DIR/.git" ]]; then
    echo "[1/7] Cloning pinned mesa-switch dependency..."
    git clone --filter=blob:none "$MESA_REPO" "$MESA_DIR"
else
    echo "[1/7] Reusing mesa-switch checkout..."
fi

git -C "$MESA_DIR" fetch --quiet origin "$MESA_PIN"
git -C "$MESA_DIR" checkout --quiet --detach "$MESA_PIN"
actual_mesa_pin="$(git -C "$MESA_DIR" rev-parse HEAD)"
if [[ "$actual_mesa_pin" != "$MESA_PIN" ]]; then
    echo "error: mesa-switch pin mismatch: $actual_mesa_pin" >&2
    exit 1
fi

git -C "$MESA_DIR" restore --source="$MESA_PIN" -- Docker.rust build-switch.sh
if ! git -C "$MESA_DIR" apply --check "$MESA_PATCH"; then
    echo "error: Mesa integration patch no longer applies to $MESA_PIN" >&2
    exit 1
fi
git -C "$MESA_DIR" apply "$MESA_PATCH"
echo "      mesa-switch: $actual_mesa_pin"

need_mesa_build=0
if [[ ! -f "$VULKAN_ARCHIVE" || "${MKW_M3_FORCE_MESA_REBUILD:-0}" == "1" ]]; then
    need_mesa_build=1
fi
if ! docker image inspect "$MESA_IMAGE" >/dev/null 2>&1; then
    need_mesa_build=1
fi

if ((need_mesa_build != 0)); then
    echo "[2/7] Building loaderless NVK/Mesa for Switch..."
    (
        cd "$MESA_DIR"
        MESA_SWITCH_IMAGE_NAME="$MESA_IMAGE" \
        MESA_SWITCH_RUST_TARGET="$RUST_TARGET" \
        ./build-switch.sh
    )
else
    echo "[2/7] Reusing proven loaderless NVK archive and build image."
fi

if [[ ! -f "$VULKAN_ARCHIVE" ]]; then
    echo "error: Mesa build did not produce $VULKAN_ARCHIVE" >&2
    exit 1
fi
if ! docker image inspect "$MESA_IMAGE" >/dev/null 2>&1; then
    echo "error: expected Mesa build image '$MESA_IMAGE' is missing" >&2
    exit 1
fi

if [[ ! -d "$DAWN_DIR/.git" ]]; then
    echo "[3/7] Cloning pinned dawn-switch dependency..."
    git clone --filter=blob:none "$DAWN_REPO" "$DAWN_DIR"
else
    echo "[3/7] Reusing dawn-switch checkout..."
fi

git -C "$DAWN_DIR" fetch --quiet origin "$DAWN_PIN"
git -C "$DAWN_DIR" checkout --quiet --detach "$DAWN_PIN"
actual_dawn_pin="$(git -C "$DAWN_DIR" rev-parse HEAD)"
if [[ "$actual_dawn_pin" != "$DAWN_PIN" ]]; then
    echo "error: dawn-switch pin mismatch: $actual_dawn_pin" >&2
    exit 1
fi
echo "      dawn-switch: $actual_dawn_pin"

echo "      initializing public Dawn third-party submodules..."
git -C "$DAWN_DIR" \
    -c submodule.third_party/angle.update=none \
    -c submodule.third_party/swiftshader.update=none \
    submodule update --init --recursive --jobs "$JOBS" --depth 1

if [[ ! -e "$ABSEIL_DIR/.git" ]]; then
    echo "error: Dawn Abseil submodule was not initialized: $ABSEIL_DIR" >&2
    exit 1
fi

# Dawn's pinned Abseil assumes glibc/POSIX details that differ under Switch
# newlib. Restore the exact submodule revision and apply only our narrow
# Horizon portability delta.
git -C "$ABSEIL_DIR" restore --source=HEAD -- \
    absl/base/internal/sysinfo.cc \
    absl/base/internal/thread_identity.cc \
    absl/debugging/internal/elf_mem_image.h \
    absl/time/internal/cctz/src/time_zone_libc.cc
if ! git -C "$ABSEIL_DIR" apply --check "$ABSEIL_PATCH"; then
    echo "error: Abseil Switch/newlib patch no longer applies" >&2
    exit 1
fi
git -C "$ABSEIL_DIR" apply "$ABSEIL_PATCH"

# The public fork contains Switch NWindow/Vulkan support, but its sample CMake
# linkage assumes a different NVK package shape. Keep the upstream pin exact,
# then apply only our narrow static-link integration delta.
git -C "$DAWN_DIR" restore --source="$DAWN_PIN" -- src/dawn/native/CMakeLists.txt
if ! git -C "$DAWN_DIR" apply --check "$DAWN_PATCH"; then
    echo "error: Dawn integration patch no longer applies to $DAWN_PIN" >&2
    exit 1
fi
git -C "$DAWN_DIR" apply "$DAWN_PATCH"
if ! git -C "$DAWN_DIR" apply --check "$AURORA_TARGET_PATCH"; then
    echo "error: Dawn Aurora target patch no longer applies to $DAWN_PIN" >&2
    exit 1
fi
git -C "$DAWN_DIR" apply "$AURORA_TARGET_PATCH"

# Reuse the fork's proven Switch NRO wrapper, but replace its triangle source
# with our Nintendo-data-free Aurora GX probe. The external CMake subdirectory
# adds the pinned Aurora GX static library to the same Dawn/NVK link group.
mkdir -p "$DAWN_DIR/src/dawn/switch"
cp "$PROBE_SOURCE" "$DAWN_DIR/src/dawn/switch/triangle_nro.cpp"

if [[ "${MKW_M3_FORCE_HLE_FIFO_RECONFIGURE:-0}" == "1" ]]; then
    rm -rf "$DAWN_BUILD_DIR"
    mkdir -p "$DAWN_BUILD_DIR"
fi

echo "[4/7] Configuring pinned HleFifoWrite + Aurora GX + Dawn/NVK path..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -e MKW_M3_JOBS="$JOBS" \
    -e MESA_SWITCH_RUST_TARGET="$RUST_TARGET" \
    -v "$MESA_DIR:/mesa:ro" \
    -v "$DAWN_DIR:/dawn" \
    -v "$DAWN_BUILD_DIR:/build" \
    -v "$WII_DIR:/wiicompiled:ro" \
    -v "$PROBE_DIR:/probe:ro" \
    -v "$ROOT_DIR:/repo:ro" \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        export DEVKITPRO=/opt/devkitpro

        command -v cmake >/dev/null
        command -v ninja >/dev/null
        command -v python3 >/dev/null

        target_libdir="$(rustc --print target-libdir --target="$MESA_SWITCH_RUST_TARGET")"
        stems=(
            std
            panic_unwind
            object
            memchr
            addr2line
            gimli
            rustc_demangle
            std_detect
            hashbrown
            rustc_std_workspace_alloc
            miniz_oxide
            unwind
            cfg_if
            libc
            alloc
            rustc_std_workspace_core
            core
            compiler_builtins
        )

        shopt -s nullglob
        rust_libs=()
        for stem in "${stems[@]}"; do
            matches=("$target_libdir/lib${stem}-"*.rlib)
            if (("${#matches[@]}" != 1)); then
                echo "error: expected exactly one Rust std archive for $stem; found ${#matches[@]}" >&2
                exit 1
            fi
            rust_libs+=("${matches[0]}")
        done

        adler_lib=""
        adler_stem=""
        for candidate in adler2 adler; do
            matches=("$target_libdir/lib${candidate}-"*.rlib)
            if (("${#matches[@]}" > 1)); then
                echo "error: multiple Rust $candidate archives found in $target_libdir" >&2
                exit 1
            fi
            if (("${#matches[@]}" == 1)); then
                adler_lib="${matches[0]}"
                adler_stem="$candidate"
                break
            fi
        done
        if [[ -z "$adler_lib" ]]; then
            echo "error: neither Rust adler2 nor adler archive exists in $target_libdir" >&2
            exit 1
        fi
        rust_libs+=("$adler_lib")
        echo "Rust compression dependency: $adler_stem"

        compat_dir=/build/compat-libs
        mkdir -p "$compat_dir"
        cat >"$compat_dir/empty_posix.c" <<"EOF"
void wiicompiled_switch_empty_posix_archive(void) {}
EOF
        /opt/devkitpro/devkitA64/bin/aarch64-none-elf-gcc \
            -c "$compat_dir/empty_posix.c" -o "$compat_dir/empty_posix.o"
        for compat in dl rt util; do
            /opt/devkitpro/devkitA64/bin/aarch64-none-elf-ar \
                rcs "$compat_dir/lib${compat}.a" "$compat_dir/empty_posix.o"
        done

        extra_libs="m3_aurora_gx"
        for lib in "${rust_libs[@]}"; do
            if [[ -n "$extra_libs" ]]; then
                extra_libs+=";"
            fi
            extra_libs+="$lib"
        done
        extra_libs+=";expat;zstd;z;stdc++"
        extra_libs+=";$compat_dir/libdl.a;$compat_dir/librt.a;$compat_dir/libutil.a"

        cmake -S /dawn -B /build -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
            -DDAWN_PLATFORM_SWITCH=ON \
            -DDAWN_FETCH_DEPENDENCIES=ON \
            -DDAWN_ENABLE_VULKAN=ON \
            -DDAWN_ENABLE_NULL=ON \
            -DDAWN_ENABLE_OPENGLES=OFF \
            -DDAWN_ENABLE_DESKTOP_GL=OFF \
            -DDAWN_ENABLE_METAL=OFF \
            -DDAWN_ENABLE_D3D11=OFF \
            -DDAWN_ENABLE_D3D12=OFF \
            -DDAWN_ENABLE_SPIRV_VALIDATION=OFF \
            -DDAWN_USE_X11=OFF \
            -DDAWN_USE_WAYLAND=OFF \
            -DDAWN_BUILD_SAMPLES=OFF \
            -DDAWN_BUILD_TESTS=OFF \
            -DTINT_BUILD_TESTS=OFF \
            -DTINT_BUILD_CMD_TOOLS=OFF \
            -DDAWN_BUILD_MONOLITHIC_LIBRARY=STATIC \
            -DDAWN_SWITCH_BUILD_TRIANGLE_NRO=ON \
            -DDAWN_SWITCH_AURORA_PROBE_DIR=/probe \
            -DDAWN_SWITCH_AURORA_ROOT=/wiicompiled/aurora-main \
            -DM3_REPO_ROOT=/repo \
            -DM3_BUILD_RENDERED_FAST_TRACK=OFF \
            -DDAWN_SWITCH_NVK_ROOT=/mesa \
            -DDAWN_SWITCH_NVK_LIBRARY=/mesa/builddir-switch/src/nouveau/vulkan/libvulkan.a \
            "-DDAWN_SWITCH_EXTRA_LIBRARIES=$extra_libs"
    '

echo "[5/7] Building pinned HleFifoWrite -> Aurora GX triangle probe..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -e MKW_M3_JOBS="$JOBS" \
    -v "$MESA_DIR:/mesa:ro" \
    -v "$DAWN_DIR:/dawn:ro" \
    -v "$DAWN_BUILD_DIR:/build" \
    -v "$WII_DIR:/wiicompiled:ro" \
    -v "$PROBE_DIR:/probe:ro" \
    -v "$ROOT_DIR:/repo:ro" \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        export DEVKITPRO=/opt/devkitpro
        cmake --build /build --target dawn_switch_triangle_nro -j"$MKW_M3_JOBS"
    '

echo "[6/7] Collecting NRO..."
built_nro="$(find "$DAWN_BUILD_DIR" -type f -name 'dawn_switch_triangle.nro' -print -quit)"
if [[ -z "$built_nro" || ! -f "$built_nro" ]]; then
    echo "error: HleFifoWrite/Aurora build did not produce dawn_switch_triangle.nro" >&2
    exit 1
fi
cp "$built_nro" "$OUTPUT"

echo "[7/7] Probe ready."
echo "NRO: $OUTPUT"
echo
echo "Copy it to:"
echo "  /switch/WiiCompiled-Switch-m3-hle-fifo-aurora-probe/WiiCompiled-Switch-m3-hle-fifo-aurora-probe.nro"
echo
echo "Expected hardware result:"
echo "  - a large RGB triangle appears from bytewise pinned HleFifoWrite -> Aurora GX -> Dawn/NVK;"
echo "  - press + to exit;"
echo "  - send back /switch/WiiCompiled-Switch/m3-hle-fifo-aurora-probe.txt."
