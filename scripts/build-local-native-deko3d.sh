#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly REF_DIR="$ROOT_DIR/third_party/DriftDroid"
readonly DEPS_DIR="${MKW_DEKO3D_DEPS_DIR:-$ROOT_DIR/.deps/native-deko3d}"
readonly WORK_DIR="${MKW_DEKO3D_WORK_DIR:-$DEPS_DIR/driftdroid}"
readonly BUILD_DIR="${MKW_DEKO3D_BUILD_DIR:-$WORK_DIR/build-switch-release}"
readonly OUTPUT_DIR="${MKW_DEKO3D_OUTPUT_DIR:-$ROOT_DIR/local-product/native-deko3d}"
readonly JOBS="${MKW_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
readonly TRANSLATED_JOBS="${MKW_TRANSLATED_COMPILE_JOBS:-2}"
readonly DEKO3D_DEBUG="${AURORA_SWITCH_DEKO3D_DEBUG:-OFF}"
readonly PGO_MODE="${MKW_PGO:-}"
readonly REF_PIN="8b33d31016a8b8735fe35b4fb633c491d33774c6"
readonly DAWN_BASE_PIN="13abc3bc8ea2d3c2050f9e77a12d012108ceee24"
readonly UAM_PIN="f8c9eef01ffe06334d530393d636d69e2b52744b"
readonly DAWN_EXCLUDE_EGL="third_party/EGL-Registry/src"
readonly DAWN_EXCLUDE_ABSEIL="third_party/abseil-cpp"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

for tool in git cmake ninja nacptool elf2nro sha256sum; do
    need "$tool"
done

if [[ -z "${DEVKITPRO:-}" || ! -f "$DEVKITPRO/cmake/Switch.cmake" ]]; then
    echo "error: DEVKITPRO must point at a devkitPro install containing cmake/Switch.cmake" >&2
    exit 2
fi

# Default to a one-command local build: prepare the exact public graphics
# dependencies first. Advanced users can reuse an already attributed dependency
# tree with MKW_SKIP_DEKO3D_DEPS=1.
if [[ "${MKW_SKIP_DEKO3D_DEPS:-0}" != "1" ]]; then
    bash "$ROOT_DIR/scripts/prepare-native-deko3d-deps.sh"
fi

if [[ "${MKW_SKIP_TRANSLATE:-0}" != "1" ]]; then
    "$ROOT_DIR/scripts/prepare-local-native-deko3d-product.sh"
elif [[ ! -f "$WORK_DIR/generated/build_shards/shards.cmake" ]]; then
    echo "error: MKW_SKIP_TRANSLATE=1 but no prepared translation exists in $WORK_DIR" >&2
    exit 2
fi

DAWN_SOURCE_DIR="${AURORA_SWITCH_DAWN_SOURCE_DIR:-$DEPS_DIR/dawn}"
DAWN_BUILD_DIR="${AURORA_SWITCH_DAWN_BUILD_DIR:-$DEPS_DIR/dawn-build-switch}"
UAM_ROOT="${AURORA_SWITCH_UAM_ROOT:-$DEPS_DIR/uam}"
UAM_LIB="${AURORA_SWITCH_UAM_LIB:-$UAM_ROOT/build-switch/libuam.a}"
readonly DAWN_SOURCE_DIR DAWN_BUILD_DIR UAM_ROOT UAM_LIB
readonly DAWN_ARCHIVE="$DAWN_BUILD_DIR/src/dawn/native/libwebgpu_dawn.a"
readonly DAWN_PATCH="$REF_DIR/aurora-main/cmake/patches/dawn-nintendo-switch-platform.patch"
readonly AURORA_ABSEIL_PATCH="$REF_DIR/aurora-main/cmake/patches/abseil-nintendo-switch-platform.patch"
readonly DEP_RECEIPT="$DEPS_DIR/native-deko3d-toolchain.receipt"

dawn_patch_args=(
    --exclude="$DAWN_EXCLUDE_EGL"
    --exclude="$DAWN_EXCLUDE_ABSEIL"
)

if [[ ! -d "$REF_DIR/.git" && ! -f "$REF_DIR/.git" ]]; then
    echo "error: DriftDroid reference submodule is missing" >&2
    exit 3
fi
if [[ "$(git -C "$REF_DIR" rev-parse HEAD)" != "$REF_PIN" ]]; then
    echo "error: DriftDroid reference pin mismatch" >&2
    exit 3
fi
if [[ ! -d "$DAWN_SOURCE_DIR/.git" ]]; then
    echo "error: attributed Switch Dawn checkout not found: $DAWN_SOURCE_DIR" >&2
    exit 3
fi
actual_dawn_head="$(git -C "$DAWN_SOURCE_DIR" rev-parse HEAD)"
if [[ "$actual_dawn_head" != "$DAWN_BASE_PIN" ]]; then
    echo "error: Dawn base mismatch: expected $DAWN_BASE_PIN, got $actual_dawn_head" >&2
    exit 3
fi
if [[ ! -f "$DAWN_PATCH" || ! -f "$AURORA_ABSEIL_PATCH" ]]; then
    echo "error: pinned DriftDroid Switch dependency patches are missing" >&2
    exit 3
fi
if ! git -C "$DAWN_SOURCE_DIR" apply --reverse --check "${dawn_patch_args[@]}" "$DAWN_PATCH" >/dev/null 2>&1; then
    echo "error: pinned DriftDroid Switch/Deko3D patch is not exactly applied to Dawn" >&2
    exit 3
fi
# The separate Abseil patch is intentionally NOT applied to Dawn's own Abseil
# submodule. DriftDroid applies it later to Aurora's FetchContent Abseil 20240722.0.
if [[ ! -f "$DAWN_ARCHIVE" ]]; then
    echo "error: native Deko3D Dawn archive not found: $DAWN_ARCHIVE" >&2
    exit 3
fi
if [[ ! -d "$UAM_ROOT/.git" ]]; then
    echo "error: attributed libuam checkout not found: $UAM_ROOT" >&2
    exit 3
fi
uam_head="$(git -C "$UAM_ROOT" rev-parse HEAD)"
if [[ "$uam_head" != "$UAM_PIN" ]]; then
    echo "error: libuam pin mismatch: expected $UAM_PIN, got $uam_head" >&2
    exit 3
fi
if [[ ! -f "$UAM_LIB" ]]; then
    echo "error: pinned libuam static library not found: $UAM_LIB" >&2
    exit 3
fi
if [[ ! -f "$DEP_RECEIPT" ]]; then
    echo "error: dependency receipt is missing: $DEP_RECEIPT" >&2
    exit 3
fi

uam_diff_sha256="$(git -C "$UAM_ROOT" diff HEAD --binary | sha256sum | awk '{print $1}')"
dawn_diff_sha256="$(git -C "$DAWN_SOURCE_DIR" diff HEAD --binary | sha256sum | awk '{print $1}')"

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

cat <<EOF_CONFIG
==> Configuring optimized Switch product
    backend       : Dawn/WebGPU -> native Deko3D
    Dawn base     : $actual_dawn_head
    UAM pin       : $uam_head
    Deko3D debug  : $DEKO3D_DEBUG
    section order : ON
    PGO           : ${PGO_MODE:-off}
    build dir     : $BUILD_DIR
EOF_CONFIG

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
receipt="$OUTPUT_DIR/WiiCompiled-Switch-native-deko3d.dependencies.txt"

nacptool --create \
    "WiiCompiled-Switch Native Deko3D" \
    "WiiCompiled-Switch" \
    "0.2.0-deko3d" \
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

deko3d_name="libdeko3d.a"
if [[ "$DEKO3D_DEBUG" == "ON" ]]; then
    deko3d_name="libdeko3dd.a"
fi
deko3d_lib="$(find "$DEVKITPRO" -type f -name "$deko3d_name" -print -quit 2>/dev/null || true)"
deko3d_sha256="not-found"
if [[ -n "$deko3d_lib" && -f "$deko3d_lib" ]]; then
    deko3d_sha256="$(sha256sum "$deko3d_lib" | awk '{print $1}')"
fi
compiler="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-g++"
compiler_version="not-found"
if [[ -x "$compiler" ]]; then
    compiler_version="$($compiler --version | head -n 1)"
fi

{
    echo "driftdroid_reference=$REF_PIN"
    echo "dawn_release=v20260603.191052"
    echo "dawn_base_commit=$actual_dawn_head"
    echo "dawn_patch_sha256=$(sha256sum "$DAWN_PATCH" | awk '{print $1}')"
    echo "dawn_patch_excluded_gitlinks=$DAWN_EXCLUDE_EGL,$DAWN_EXCLUDE_ABSEIL"
    echo "dawn_worktree_diff_sha256=$dawn_diff_sha256"
    echo "dawn_archive_sha256=$(sha256sum "$DAWN_ARCHIVE" | awk '{print $1}')"
    echo "aurora_abseil_patch_sha256=$(sha256sum "$AURORA_ABSEIL_PATCH" | awk '{print $1}')"
    echo "uam_repo=https://github.com/averne/libuam.git"
    echo "uam_commit=$uam_head"
    echo "uam_worktree_diff_sha256=$uam_diff_sha256"
    echo "uam_archive_sha256=$(sha256sum "$UAM_LIB" | awk '{print $1}')"
    echo "dependency_receipt_sha256=$(sha256sum "$DEP_RECEIPT" | awk '{print $1}')"
    echo "deko3d_mode=$DEKO3D_DEBUG"
    echo "deko3d_library=${deko3d_lib:-not-found}"
    echo "deko3d_library_sha256=$deko3d_sha256"
    echo "compiler=$compiler_version"
    echo "cmake=$(cmake --version | head -n 1)"
    echo "ninja=$(ninja --version)"
    echo "pgo=${PGO_MODE:-off}"
    echo "translated_compile_jobs=$TRANSLATED_JOBS"
    echo "nro_sha256=$(awk '{print $1}' "$nro.sha256")"
} > "$receipt"

cat <<EOF_DONE
Optimized native Deko3D candidate built.
  NRO       : $nro
  SHA-256   : $(awk '{print $1}' "$nro.sha256")
  debug ELF : $debug_elf
  deps      : $receipt

This output is private/local and ignored by Git. Launch through hbmenu title override
(full memory). Keep the dependency receipt with every hardware report. Do not claim
playability/FPS until this exact NRO SHA is verified on hardware.
EOF_DONE
