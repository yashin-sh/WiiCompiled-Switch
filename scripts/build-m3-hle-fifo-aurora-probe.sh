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
readonly WII_GXGEOMETRY_PATCH="$ROOT_DIR/patches/wiicompiled/m3-wiicompiled-switch-build.patch"
readonly PROBE_DIR="$ROOT_DIR/m3-hle-fifo-probe"
readonly PROBE_SOURCE="$PROBE_DIR/source/main.cpp"
readonly OUTPUT_DIR="$PROBE_DIR"
readonly OUTPUT="$OUTPUT_DIR/WiiCompiled-Switch-m3-hle-fifo-aurora-probe.nro"

readonly MESA_IMAGE="${MKW_M3_MESA_IMAGE:-wiicompiled-m3-mesa-b297e230-v3}"
readonly RUST_TARGET="aarch64-unknown-linux-gnu"
readonly JOBS="${MKW_JOBS:-4}"
readonly VULKAN_ARCHIVE="$MESA_DIR/builddir-switch/src/nouveau/vulkan/libvulkan.a"
RENDERED_MODE="${MKW_M3_BUILD_RENDERED_FAST_TRACK:-OFF}"
DISCOVERY_MODE="${MKW_DISCOVERY_SCAN_MODE:-OFF}"
CAPTURE_MODE="${MKW_RENDERED_FIFO_CAPTURE:-OFF}"
for mode in RENDERED_MODE DISCOVERY_MODE CAPTURE_MODE; do
    case "${!mode}" in
        ON|1) printf -v "$mode" %s ON ;;
        OFF|0) printf -v "$mode" %s OFF ;;
        *) echo "error: $mode must be ON/OFF or 1/0" >&2; exit 2 ;;
    esac
done
readonly RENDERED_MODE DISCOVERY_MODE CAPTURE_MODE

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}


# Leave an already configured dependency untouched, including its mtimes.
# Refuse other local edits rather than restoring a dependency over them.
apply_pinned_patch() {
    local checkout="$1" patch="$2" predecessor=""
    shift 2
    if [[ "${1:-}" == "--after-patch" ]]; then
        predecessor="$2"
        shift 2
    fi
    if git -C "$checkout" apply --reverse --check "$patch" >/dev/null 2>&1; then
        return 0
    fi
    if ! git -C "$checkout" diff --cached --quiet -- "$@"; then
        echo "error: staged patch paths have local changes in $checkout" >&2
        return 1
    fi
    if ! git -C "$checkout" diff --quiet -- "$@"; then
        if [[ -z "$predecessor" ]]; then
            echo "error: patch paths have local changes in $checkout; refusing to overwrite them" >&2
            return 1
        fi
        # Dawn's second patch touches the first patch's file. Accept exactly
        # HEAD plus that explicit predecessor, using only temporary storage.
        local expected_dir file matches=1
        expected_dir="$(mktemp -d)"
        for file in "$@"; do
            mkdir -p "$(dirname "$expected_dir/$file")"
            if ! git -C "$checkout" show "HEAD:$file" >"$expected_dir/$file"; then
                matches=0
                break
            fi
        done
        if ((matches != 0)) && git -C "$expected_dir" apply "$predecessor"; then
            for file in "$@"; do
                if ! cmp -s "$expected_dir/$file" "$checkout/$file"; then
                    matches=0
                    break
                fi
            done
        else
            matches=0
        fi
        rm -rf -- "$expected_dir"
        if ((matches == 0)); then
            echo "error: patch paths differ from the authorized predecessor in $checkout" >&2
            return 1
        fi
    fi
    if ! git -C "$checkout" apply --check "$patch"; then
        echo "error: integration patch no longer applies: $patch" >&2
        return 1
    fi
    git -C "$checkout" apply "$patch"
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
if [[ ! -f "$WII_GXGEOMETRY_PATCH" ]]; then
    echo "error: missing WiiCompiled GXGeometry patch: $WII_GXGEOMETRY_PATCH" >&2
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

if ! git -C "$WII_DIR" cat-file -e "${WII_PIN}^{commit}" 2>/dev/null; then
    git -C "$WII_DIR" fetch --quiet origin "$WII_PIN"
fi
if [[ "$(git -C "$WII_DIR" rev-parse HEAD)" != "$WII_PIN" ]]; then
    git -C "$WII_DIR" checkout --quiet --detach "$WII_PIN"
fi
actual_wii_pin="$(git -C "$WII_DIR" rev-parse HEAD)"
if [[ "$actual_wii_pin" != "$WII_PIN" ]]; then
    echo "error: WiiCompiled pin mismatch: $actual_wii_pin" >&2
    exit 1
fi
echo "      WiiCompiled: $actual_wii_pin"

# aurora-main declares a C++ convenience overload inside an extern "C"
# block, which cannot compile as C++. Reuse or apply our linkage fix at the
# exact pin, refusing conflicting local edits (endianness is explicit at callers).
apply_pinned_patch "$WII_DIR" "$WII_GXGEOMETRY_PATCH" \
    aurora-main/include/dolphin/gx/GXGeometry.h \
    runtime/include/abi_bridge.h \
    runtime/include/gx_guest_write.h \
    runtime/include/runtime_config.h \
    runtime/include/runtime_log.h \
    runtime/include/system_bridge.h \
    runtime/src/hle/gx/gx_internal.h \
    runtime/src/hle/gx/gx_stream_common.h \
    runtime/src/hle/gx/gx_dl.cpp

if [[ ! -e "$MESA_DIR/.git" ]]; then
    echo "[1/7] Cloning pinned mesa-switch dependency..."
    git clone --filter=blob:none "$MESA_REPO" "$MESA_DIR"
else
    echo "[1/7] Reusing mesa-switch checkout..."
fi

if ! git -C "$MESA_DIR" cat-file -e "${MESA_PIN}^{commit}" 2>/dev/null; then
    git -C "$MESA_DIR" fetch --quiet origin "$MESA_PIN"
fi
if [[ "$(git -C "$MESA_DIR" rev-parse HEAD)" != "$MESA_PIN" ]]; then
    git -C "$MESA_DIR" checkout --quiet --detach "$MESA_PIN"
fi
actual_mesa_pin="$(git -C "$MESA_DIR" rev-parse HEAD)"
if [[ "$actual_mesa_pin" != "$MESA_PIN" ]]; then
    echo "error: mesa-switch pin mismatch: $actual_mesa_pin" >&2
    exit 1
fi

apply_pinned_patch "$MESA_DIR" "$MESA_PATCH" \
    Docker.rust \
    build-switch.sh
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

if [[ ! -e "$DAWN_DIR/.git" ]]; then
    echo "[3/7] Cloning pinned dawn-switch dependency..."
    git clone --filter=blob:none "$DAWN_REPO" "$DAWN_DIR"
else
    echo "[3/7] Reusing dawn-switch checkout..."
fi

if ! git -C "$DAWN_DIR" cat-file -e "${DAWN_PIN}^{commit}" 2>/dev/null; then
    git -C "$DAWN_DIR" fetch --quiet origin "$DAWN_PIN"
fi
if [[ "$(git -C "$DAWN_DIR" rev-parse HEAD)" != "$DAWN_PIN" ]]; then
    git -C "$DAWN_DIR" checkout --quiet --detach "$DAWN_PIN"
fi
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
# newlib. Reuse or apply our Horizon portability delta at the pinned revision,
# refusing conflicting local edits.
apply_pinned_patch "$ABSEIL_DIR" "$ABSEIL_PATCH" \
    absl/base/internal/sysinfo.cc \
    absl/base/internal/thread_identity.cc \
    absl/debugging/internal/elf_mem_image.h \
    absl/time/internal/cctz/src/time_zone_libc.cc

# The public fork contains Switch NWindow/Vulkan support, but its sample CMake
# linkage assumes a different NVK package shape. Keep the upstream pin exact,
# then reuse or apply our static-link delta without overwriting local edits.
apply_pinned_patch "$DAWN_DIR" "$DAWN_PATCH" src/dawn/native/CMakeLists.txt
apply_pinned_patch "$DAWN_DIR" "$AURORA_TARGET_PATCH" \
    --after-patch "$DAWN_PATCH" src/dawn/native/CMakeLists.txt

# Reuse the fork's proven Switch NRO wrapper, but replace its triangle source
# with our Nintendo-data-free Aurora GX probe. The external CMake subdirectory
# adds the pinned Aurora GX static library to the same Dawn/NVK link group.
mkdir -p "$DAWN_DIR/src/dawn/switch"
if ! cmp -s "$PROBE_SOURCE" "$DAWN_DIR/src/dawn/switch/triangle_nro.cpp"; then
    cp "$PROBE_SOURCE" "$DAWN_DIR/src/dawn/switch/triangle_nro.cpp"
fi

if [[ "${MKW_M3_FORCE_HLE_FIFO_RECONFIGURE:-0}" == "1" ]]; then
    build_path="$(realpath -m -- "$DAWN_BUILD_DIR")"
    if [[ "$build_path" == "/" ]]; then
        echo "error: refusing to clean the filesystem root" >&2
        exit 1
    fi
    for protected_root in "$ROOT_DIR" "$MESA_DIR" "$DAWN_DIR"; do
        protected_path="$(realpath -m -- "$protected_root")"
        if [[ "$protected_path" == "$build_path" || "$protected_path" == "$build_path/"* ]]; then
            echo "error: build cleanup would remove source files: $DAWN_BUILD_DIR" >&2
            exit 1
        fi
    done
    rm -rf -- "$DAWN_BUILD_DIR"
    mkdir -p "$DAWN_BUILD_DIR"
fi

echo "[4/7] Configuring pinned HleFifoWrite + Aurora GX + Dawn/NVK path..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -e MKW_M3_JOBS="$JOBS" \
    -e MESA_SWITCH_RUST_TARGET="$RUST_TARGET" \
    -e MKW_M3_BUILD_RENDERED_FAST_TRACK="$RENDERED_MODE" \
    -e MKW_RENDERED_FIFO_CAPTURE="$CAPTURE_MODE" \
    -e MKW_DISCOVERY_SCAN_MODE="$DISCOVERY_MODE" \
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
            -DM3_BUILD_RENDERED_FAST_TRACK="$MKW_M3_BUILD_RENDERED_FAST_TRACK" \
            -DMKW_DISCOVERY_SCAN_MODE="$MKW_DISCOVERY_SCAN_MODE" \
            -DMKW_RENDERED_FIFO_CAPTURE="$MKW_RENDERED_FIFO_CAPTURE" \
            -DDAWN_SWITCH_NVK_ROOT=/mesa \
            -DDAWN_SWITCH_NVK_LIBRARY=/mesa/builddir-switch/src/nouveau/vulkan/libvulkan.a \
            "-DDAWN_SWITCH_EXTRA_LIBRARIES=$extra_libs"
    '

if [[ "$RENDERED_MODE" == "ON" ]]; then
    echo "Rendered Dawn/Aurora/NVK build environment prepared."
    exit 0
fi

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
built_nro_listing="$(find "$DAWN_BUILD_DIR" -type f -name 'dawn_switch_triangle.nro' -print)"
mapfile -t built_nros <<< "$built_nro_listing"
if (( ${#built_nros[@]} != 1 )) || [[ ! -f "${built_nros[0]}" ]]; then
    echo "error: HleFifoWrite/Aurora build did not produce exactly one dawn_switch_triangle.nro" >&2
    exit 1
fi
built_nro="${built_nros[0]}"
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
