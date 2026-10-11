#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly WORK_DIR="${MKW_DEKO3D_WORK_DIR:-$ROOT_DIR/.deps/native-deko3d/driftdroid}"
readonly BUILD_DIR="${MKW_DEKO3D_BUILD_DIR:-$WORK_DIR/build-switch-release}"
readonly OUTPUT_DIR="${MKW_DEKO3D_OUTPUT_DIR:-$ROOT_DIR/local-product/native-deko3d}"
readonly JOBS="${MKW_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
readonly TRANSLATED_JOBS="${MKW_TRANSLATED_COMPILE_JOBS:-2}"
readonly DEKO3D_DEBUG="${AURORA_SWITCH_DEKO3D_DEBUG:-OFF}"
readonly PGO_MODE="${MKW_PGO:-}"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need cmake
need ninja
need nacptool
need elf2nro
need sha256sum

if [[ -z "${DEVKITPRO:-}" || ! -f "$DEVKITPRO/cmake/Switch.cmake" ]]; then
    echo "error: DEVKITPRO must point at a devkitPro install containing cmake/Switch.cmake" >&2
    exit 2
fi

if [[ "${MKW_SKIP_TRANSLATE:-0}" != "1" ]]; then
    "$ROOT_DIR/scripts/prepare-local-native-deko3d-product.sh"
elif [[ ! -f "$WORK_DIR/generated/build_shards/shards.cmake" ]]; then
    echo "error: MKW_SKIP_TRANSLATE=1 but no prepared translation exists in $WORK_DIR" >&2
    exit 2
fi

# DriftDroid's proven Switch backend uses a patched Dawn archive plus UAM. Keep
# these outside this repository: they are toolchain dependencies, not generated
# game content. The defaults match the pinned reference implementation.
DAWN_SOURCE_DIR="${AURORA_SWITCH_DAWN_SOURCE_DIR:-$HOME/switch-dawn/dawn}"
DAWN_BUILD_DIR="${AURORA_SWITCH_DAWN_BUILD_DIR:-$HOME/switch-dawn/build-switch}"
UAM_LIB="${AURORA_SWITCH_UAM_LIB:-$HOME/uam/build-switch/libuamcore.a}"
readonly DAWN_SOURCE_DIR DAWN_BUILD_DIR UAM_LIB

if [[ ! -d "$DAWN_SOURCE_DIR/include" ]]; then
    echo "error: patched Switch Dawn source not found: $DAWN_SOURCE_DIR" >&2
    echo "       set AURORA_SWITCH_DAWN_SOURCE_DIR to the pinned patched Dawn source" >&2
    exit 3
fi
if [[ ! -f "$DAWN_BUILD_DIR/src/dawn/native/libwebgpu_dawn.a" ]]; then
    echo "error: patched Switch Dawn archive not found under: $DAWN_BUILD_DIR" >&2
    echo "       expected src/dawn/native/libwebgpu_dawn.a" >&2
    exit 3
fi
if [[ ! -f "$UAM_LIB" ]]; then
    echo "error: UAM static library not found: $UAM_LIB" >&2
    exit 3
fi

mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

configure=(
    -S "$WORK_DIR/runtime"
    -B "$BUILD_DIR"
    -G Ninja
    -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake"
    -DCMAKE_BUILD_TYPE=Release
    -DAURORA_SWITCH_DAWN_SOURCE_DIR="$DAWN_SOURCE_DIR"
    -DAURORA_SWITCH_DAWN_BUILD_DIR="$DAWN_BUILD_DIR"
    -DAURORA_SWITCH_UAM_LIB="$UAM_LIB"
    -DAURORA_SWITCH_DEKO3D_DEBUG="$DEKO3D_DEBUG"
    -DMKW_SWITCH_SECTION_ORDERING=ON
    -DMKW_TRANSLATED_COMPILE_JOBS="$TRANSLATED_JOBS"
)
if [[ -n "$PGO_MODE" ]]; then
    configure+=("-DMKW_PGO=$PGO_MODE")
fi

cat <<EOF
==> Configuring optimized Switch product
    backend       : Dawn/WebGPU -> native Deko3D
    Deko3D debug  : $DEKO3D_DEBUG
    section order : ON
    PGO           : ${PGO_MODE:-off}
    build dir     : $BUILD_DIR
EOF

cmake "${configure[@]}"
cmake --build "$BUILD_DIR" --target mkw_release --parallel "$JOBS"

elf=""
for candidate in "$BUILD_DIR/WiiCompiled.elf" "$BUILD_DIR/WiiCompiled"; do
    if [[ -f "$candidate" ]]; then
        elf="$candidate"
        break
    fi
done
if [[ -z "$elf" ]]; then
    elf="$(find "$BUILD_DIR" -maxdepth 3 -type f \( -name 'WiiCompiled.elf' -o -name 'WiiCompiled' \) -print -quit)"
fi
if [[ -z "$elf" || ! -f "$elf" ]]; then
    echo "error: native WiiCompiled ELF was not produced by the Switch build" >&2
    exit 4
fi

package_dir="$BUILD_DIR/nro-package"
romfs_dir="$package_dir/romfs"
rm -rf "$package_dir"
mkdir -p "$romfs_dir/wii_bootstrap" "$OUTPUT_DIR"

cp "$WORK_DIR/runtime/assets/dsp/dsp_coef.bin" "$romfs_dir/dsp_coef.bin"
cp "$WORK_DIR/runtime/assets/pipeline/initial_pipeline_cache.db" "$romfs_dir/initial_pipeline_cache.db"
cp "$WORK_DIR/runtime/assets/certs/cacert.pem" "$romfs_dir/cacert.pem"
cp -R "$WORK_DIR/runtime/assets/wii/." "$romfs_dir/wii_bootstrap/"

nacp="$package_dir/WiiCompiled-Switch-native-deko3d.nacp"
nro="$OUTPUT_DIR/WiiCompiled-Switch-native-deko3d.nro"
debug_elf="$OUTPUT_DIR/WiiCompiled-Switch-native-deko3d.debug.elf"
package_elf="$package_dir/WiiCompiled-Switch-native-deko3d.elf"

nacptool --create \
    "WiiCompiled-Switch Native Deko3D" \
    "WiiCompiled-Switch" \
    "0.1.0-deko3d" \
    "$nacp"

cp "$elf" "$debug_elf"
cp "$elf" "$package_elf"
strip_tool="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-strip"
if [[ -x "$strip_tool" ]]; then
    "$strip_tool" --strip-debug "$package_elf"
fi

elf2nro "$package_elf" "$nro" \
    --nacp="$nacp" \
    --romfsdir="$romfs_dir"

sha256sum "$nro" > "$nro.sha256"
sha256sum "$debug_elf" > "$debug_elf.sha256"

cat <<EOF
Optimized native Deko3D candidate built.
  NRO       : $nro
  SHA-256   : $(awk '{print $1}' "$nro.sha256")
  debug ELF : $debug_elf

This output is private/local and ignored by Git. Launch through hbmenu title override
(full memory). Do not claim playability/FPS until this exact SHA is verified on hardware.
EOF
