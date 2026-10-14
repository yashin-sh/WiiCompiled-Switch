#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly REF_DIR="$ROOT_DIR/third_party/DriftDroid"
readonly REF_PIN="8b33d31016a8b8735fe35b4fb633c491d33774c6"
readonly DEPS_DIR="${MKW_DEKO3D_DEPS_DIR:-$ROOT_DIR/.deps/native-deko3d}"
readonly JOBS="${MKW_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

readonly DAWN_REPO="https://github.com/google/dawn.git"
readonly DAWN_PIN="13abc3bc8ea2d3c2050f9e77a12d012108ceee24"
readonly DAWN_ABSEIL_PIN="d16e32215c3ab90ba57c2e904a5344d85c7353e4"
readonly DAWN_DIR="${AURORA_SWITCH_DAWN_SOURCE_DIR:-$DEPS_DIR/dawn}"
readonly DAWN_BUILD_DIR="${AURORA_SWITCH_DAWN_BUILD_DIR:-$DEPS_DIR/dawn-build-switch}"
readonly UAM_REPO="https://github.com/averne/libuam.git"
readonly UAM_PIN="f8c9eef01ffe06334d530393d636d69e2b52744b"
readonly UAM_DIR="${AURORA_SWITCH_UAM_ROOT:-$DEPS_DIR/uam}"
readonly UAM_BUILD_DIR="$UAM_DIR/build-switch"
readonly UAM_LIB="${AURORA_SWITCH_UAM_LIB:-$UAM_BUILD_DIR/libuam.a}"
readonly CROSS_FILE="$DEPS_DIR/uam-switch.cross"
readonly RECEIPT="$DEPS_DIR/native-deko3d-toolchain.receipt"
readonly DAWN_PATCH="$REF_DIR/aurora-main/cmake/patches/dawn-nintendo-switch-platform.patch"
readonly AURORA_ABSEIL_PATCH="$REF_DIR/aurora-main/cmake/patches/abseil-nintendo-switch-platform.patch"
readonly DAWN_ABSEIL_PATCHER="$ROOT_DIR/scripts/patch-dawn-abseil-switch.py"
readonly DAWN_SWITCH_PATCHER="$ROOT_DIR/scripts/patch-dawn-switch.py"
readonly DAWN_SWITCH_PORTABILITY_FILE="src/tint/utils/text_generator/text_generator.cc"
readonly DAWN_SWITCH_SLEEP_FILE="src/dawn/utils/SystemUtils.cpp"
readonly DAWN_SWITCH_MESSAGE_FILE="src/dawn/native/utils/WGPUHelpers.cpp"
readonly DAWN_EXCLUDE_EGL="third_party/EGL-Registry/src"
readonly DAWN_EXCLUDE_ABSEIL="third_party/abseil-cpp"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

for tool in git cmake ninja meson python3 bison flex sha256sum; do
    need "$tool"
done

# Resolve aliases before checkout/reset cleanup. Source and build overrides
# must remain separate from the public checkout, game inputs and products.
python3 - "$ROOT_DIR" "$REF_DIR" "$DEPS_DIR" "$DAWN_DIR" "$UAM_DIR" \
    "$DAWN_BUILD_DIR" "$UAM_BUILD_DIR" \
    "${MKW_LOCAL_ASSET_DIR:-$ROOT_DIR/local-product/Assets}" \
    "${MKW_DEKO3D_WORK_DIR:-$DEPS_DIR/driftdroid}" \
    "${MKW_DEKO3D_OUTPUT_DIR:-$ROOT_DIR/local-product/native-deko3d}" <<'PY'
from pathlib import Path
import subprocess
import sys

root, reference, deps, dawn, uam, dawn_build, uam_build, assets, work, output = (
    Path(p).resolve() for p in sys.argv[1:]
)
targets = (dawn, uam, dawn_build, uam_build)
protected = tuple(path.resolve() for path in (
    reference, root / "third_party/WiiCompiled", assets, work, output,
    root / "game", root / "game-data", root / "generated-game",
    root / "assets-local",
))
for target in targets:
    for path in (root, deps, *protected):
        if target == path or target in path.parents:
            raise SystemExit(f"error: dependency cleanup would remove protected path: {path}")
    for path in protected:
        if path in target.parents:
            raise SystemExit(f"error: dependency directory overlaps protected path: {path}")
for index, left in enumerate(targets):
    for right in targets[index + 1:]:
        # libuam deliberately keeps its Meson build in its source checkout.
        if left == uam and right == uam_build and left in right.parents:
            continue
        if left == right or left in right.parents or right in left.parents:
            raise SystemExit(f"error: dependency source/build directories overlap: {left}, {right}")
for path in (deps, *targets):
    if path == root or path in root.parents:
        raise SystemExit(f"error: dependency directory contains the repository: {path}")
    if root in path.parents:
        ignored = subprocess.run(["git", "-C", str(root), "check-ignore", "-q", str(path)])
        if ignored.returncode != 0:
            raise SystemExit(f"error: dependency directory inside the repository must be Git-ignored: {path}")
for path in (dawn, uam):
    if path.exists() and not (path / ".git").exists():
        if not path.is_dir() or any(path.iterdir()):
            raise SystemExit(f"error: refusing to replace an existing non-checkout dependency path: {path}")
PY

python3 - <<'PY'
import mako
print(f"mako={mako.__version__}")
PY

if [[ -z "${DEVKITPRO:-}" || ! -f "$DEVKITPRO/cmake/Switch.cmake" ]]; then
    echo "error: DEVKITPRO must point at a devkitPro install containing cmake/Switch.cmake" >&2
    exit 2
fi
if [[ ! -f "$DEVKITPRO/switchvars.sh" ]]; then
    echo "error: missing $DEVKITPRO/switchvars.sh" >&2
    exit 2
fi
if [[ ! -f "$DAWN_ABSEIL_PATCHER" ]]; then
    echo "error: missing Dawn Abseil Switch patcher: $DAWN_ABSEIL_PATCHER" >&2
    exit 2
fi
if [[ ! -f "$DAWN_SWITCH_PATCHER" ]]; then
    echo "error: missing Dawn/Tint Switch patcher: $DAWN_SWITCH_PATCHER" >&2
    exit 2
fi

"$ROOT_DIR/scripts/prepare-native-deko3d-reference.sh"
if [[ "$(git -C "$REF_DIR" rev-parse HEAD)" != "$REF_PIN" ]]; then
    echo "error: DriftDroid reference moved unexpectedly" >&2
    exit 2
fi

mkdir -p "$DEPS_DIR"

ensure_checkout() {
    local repo="$1" dir="$2" pin="$3" name="$4"
    local fresh=0 has_worktree=""

    if [[ ! -e "$dir/.git" ]]; then
        rm -rf "$dir"
        git clone --filter=blob:none --no-checkout "$repo" "$dir"
        fresh=1
    fi

    has_worktree="$(find "$dir" -mindepth 1 -maxdepth 1 ! -name .git -print -quit)"
    if ((fresh == 0)) && [[ -n "$has_worktree" ]]; then
        if ! git -C "$dir" diff --quiet || ! git -C "$dir" diff --cached --quiet || \
           [[ -n "$(git -C "$dir" ls-files --others --exclude-standard | head -n 1)" ]]; then
            echo "error: refusing to overwrite local changes in $name checkout: $dir" >&2
            exit 3
        fi
    fi

    if ! git -C "$dir" cat-file -e "${pin}^{commit}" 2>/dev/null; then
        git -C "$dir" fetch --quiet origin "$pin"
    fi
    git -C "$dir" checkout --quiet --detach "$pin"
    local actual
    actual="$(git -C "$dir" rev-parse HEAD)"
    if [[ "$actual" != "$pin" ]]; then
        echo "error: $name pin mismatch: expected $pin, got $actual" >&2
        exit 3
    fi
}

dawn_patch_args=(
    --exclude="$DAWN_EXCLUDE_EGL"
    --exclude="$DAWN_EXCLUDE_ABSEIL"
)

dawn_patch_is_applied() {
    python3 "$DAWN_SWITCH_PATCHER" "$DAWN_DIR" --verify-base-patch "$DAWN_PATCH" >/dev/null 2>&1
}

apply_dawn_patch() {
    if dawn_patch_is_applied; then
        return 0
    fi
    if ! git -C "$DAWN_DIR" apply --check "${dawn_patch_args[@]}" "$DAWN_PATCH"; then
        echo "error: DriftDroid Dawn/Deko3D patch does not apply cleanly to pinned Dawn" >&2
        exit 4
    fi
    git -C "$DAWN_DIR" apply "${dawn_patch_args[@]}" "$DAWN_PATCH"
}

assert_dawn_abseil_switch_changes() {
    local abseil_dir="$DAWN_DIR/$DAWN_EXCLUDE_ABSEIL"
    local actual changed expected
    actual="$(git -C "$abseil_dir" rev-parse HEAD)"
    if [[ "$actual" != "$DAWN_ABSEIL_PIN" ]]; then
        echo "error: Dawn Abseil pin mismatch: expected $DAWN_ABSEIL_PIN, got $actual" >&2
        exit 4
    fi

    python3 "$DAWN_ABSEIL_PATCHER" "$abseil_dir" --check

    changed="$(git -C "$abseil_dir" diff --name-only HEAD | sort -u)"
    expected=$'absl/base/internal/sysinfo.cc\nabsl/base/internal/thread_identity.cc\nabsl/debugging/internal/elf_mem_image.h\nabsl/time/internal/cctz/src/time_zone_libc.cc'
    if [[ "$changed" != "$expected" ]]; then
        echo "error: Dawn Abseil contains changes outside the four audited libnx portability files" >&2
        printf 'expected:\n%s\nactual:\n%s\n' "$expected" "$changed" >&2
        exit 4
    fi
    if ! git -C "$abseil_dir" diff --cached --quiet; then
        echo "error: unexpected staged changes in Dawn Abseil" >&2
        exit 4
    fi
    if [[ -n "$(git -C "$abseil_dir" ls-files --others --exclude-standard | head -n 1)" ]]; then
        echo "error: unexpected untracked files in Dawn Abseil" >&2
        exit 4
    fi
}

assert_only_dawn_patch_changes() {
    local changed path allowed abseil_seen=0 switch_portability_seen=0 switch_sleep_seen=0 switch_message_seen=0
    changed="$( { git -C "$DAWN_DIR" diff --name-only HEAD; git -C "$DAWN_DIR" ls-files --others --exclude-standard; } | sort -u)"
    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        if [[ "$path" == "$DAWN_EXCLUDE_EGL" ]]; then
            echo "error: excluded EGL Dawn submodule unexpectedly became dirty" >&2
            exit 4
        fi
        if [[ "$path" == "$DAWN_EXCLUDE_ABSEIL" ]]; then
            abseil_seen=1
            continue
        fi
        allowed=0
        if [[ "$path" == "$DAWN_SWITCH_PORTABILITY_FILE" ]]; then
            allowed=1
            switch_portability_seen=1
        elif [[ "$path" == "$DAWN_SWITCH_SLEEP_FILE" ]]; then
            allowed=1
            switch_sleep_seen=1
        elif [[ "$path" == "$DAWN_SWITCH_MESSAGE_FILE" ]]; then
            allowed=1
            switch_message_seen=1
        elif grep -Eq "^(--- a/|\+\+\+ b/)${path//./\.}$" "$DAWN_PATCH"; then
            allowed=1
        fi
        if ((allowed == 0)); then
            echo "error: unexpected local change in Dawn checkout: $path" >&2
            exit 4
        fi
    done <<< "$changed"

    if ((abseil_seen == 0)); then
        echo "error: expected audited Dawn Abseil libnx portability delta is missing" >&2
        exit 4
    fi
    if ((switch_portability_seen == 0)); then
        echo "error: expected audited Dawn/Tint libnx portability delta is missing" >&2
        exit 4
    fi
    if ((switch_sleep_seen == 0)); then
        echo "error: expected audited Dawn native libnx sleep delta is missing" >&2
        exit 4
    fi
    if ((switch_message_seen == 0)); then
        echo "error: expected audited Dawn bounded message normalization delta is missing" >&2
        exit 4
    fi
    assert_dawn_abseil_switch_changes
    python3 "$DAWN_SWITCH_PATCHER" "$DAWN_DIR" --check
}

echo "[1/6] Preparing static UAM library source..."
ensure_checkout "$UAM_REPO" "$UAM_DIR" "$UAM_PIN" "libuam"

cat > "$CROSS_FILE" <<EOF_CROSS
[built-in options]
buildtype = 'release'
default_library = 'static'
wrap_mode = 'nodownload'
[binaries]
c = '$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc'
cpp = '$DEVKITPRO/devkitA64/bin/aarch64-none-elf-g++'
ar = '$DEVKITPRO/devkitA64/bin/aarch64-none-elf-ar'
strip = '$DEVKITPRO/devkitA64/bin/aarch64-none-elf-strip'
pkgconfig = '$DEVKITPRO/portlibs/switch/bin/aarch64-none-elf-pkg-config'
[host_machine]
system = 'hos'
cpu_family = 'aarch64'
cpu = 'armv8a'
endian = 'little'
EOF_CROSS

set +u
# shellcheck disable=SC1090
source "$DEVKITPRO/switchvars.sh"
set -u

if [[ "${MKW_DEKO3D_FORCE_REBUILD:-0}" == "1" ]]; then
    rm -rf "$UAM_BUILD_DIR" "$DAWN_BUILD_DIR"
fi

echo "[2/6] Building pinned averne/libuam for Horizon/AArch64..."
if [[ -f "$UAM_BUILD_DIR/meson-private/coredata.dat" ]]; then
    meson setup "$UAM_BUILD_DIR" "$UAM_DIR" --cross-file "$CROSS_FILE" --reconfigure
else
    meson setup "$UAM_BUILD_DIR" "$UAM_DIR" --cross-file "$CROSS_FILE"
fi
meson compile -C "$UAM_BUILD_DIR" -j "$JOBS"
if [[ ! -f "$UAM_LIB" ]]; then
    echo "error: libuam build did not produce $UAM_LIB" >&2
    exit 5
fi

echo "[3/6] Preparing exact Dawn source..."
if [[ -e "$DAWN_DIR/.git" ]] && [[ "$(git -C "$DAWN_DIR" rev-parse HEAD 2>/dev/null || true)" == "$DAWN_PIN" ]] && dawn_patch_is_applied; then
    echo "      reusing already patched Dawn checkout"
else
    ensure_checkout "$DAWN_REPO" "$DAWN_DIR" "$DAWN_PIN" "Dawn"
fi

echo "      initializing Dawn dependencies..."
git -C "$DAWN_DIR" \
    -c submodule.third_party/angle.update=none \
    -c submodule.third_party/swiftshader.update=none \
    submodule update --init --recursive --jobs "$JOBS" --depth 1

if [[ ! -e "$DAWN_DIR/third_party/abseil-cpp/.git" ]]; then
    echo "error: Dawn Abseil dependency was not initialized" >&2
    exit 4
fi
if [[ "$(git -C "$DAWN_DIR/third_party/abseil-cpp" rev-parse HEAD)" != "$DAWN_ABSEIL_PIN" ]]; then
    echo "error: Dawn initialized an unexpected Abseil revision" >&2
    exit 4
fi

# DriftDroid's Dawn patch was exported from a worktree where EGL-Registry and
# Dawn's own Abseil submodules were dirty. Those two gitlink-only "-dirty"
# hunks are metadata noise and are deliberately excluded. DriftDroid's separate
# Aurora Abseil patch targets a different 20240722.0 source tree. Dawn's pinned
# internal Abseil needs four narrow libnx fixes observed by the public build:
# Horizon thread handles for GetTID, the no-signals pthread path for thread
# identity, disabling glibc-style ELF dynamic-linker introspection, and using
# cctz's existing newlib _timezone/tzname fallback on Switch. Pinned Tint also
# needs an audited strict-C++20 fallback for newlib's hidden isascii extension.
# Dawn also needs its hidden POSIX setenv/unsetenv declarations and a native
# Horizon sleep path because strict C++20 hides newlib's usleep declaration.
apply_dawn_patch
python3 "$DAWN_ABSEIL_PATCHER" "$DAWN_DIR/third_party/abseil-cpp"
python3 "$DAWN_SWITCH_PATCHER" "$DAWN_DIR"
if ! dawn_patch_is_applied; then
    echo "error: Dawn Deko3D patch verification failed after apply" >&2
    exit 4
fi
assert_only_dawn_patch_changes

# Deko3D consumes WGSL through Tint's desktop GLSL writer before UAM emits
# DKSH. Upstream defaults that writer from the GL backend flags, which remain
# disabled here; request its shader pipeline explicitly, including on reuse.
echo "[4/6] Configuring minimal Dawn native Deko3D backend..."
cmake -S "$DAWN_DIR" -B "$DAWN_BUILD_DIR" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DDAWN_FETCH_DEPENDENCIES=OFF \
    -DDAWN_ENABLE_DEKO3D=ON \
    -DDAWN_UAM_DIR="$UAM_DIR" \
    -DDAWN_ENABLE_VULKAN=OFF \
    -DDAWN_ENABLE_NULL=OFF \
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
    -DDAWN_BUILD_NODE_BINDINGS=OFF \
    -DDAWN_BUILD_BENCHMARKS=OFF \
    -DDAWN_BUILD_PROTOBUF=OFF \
    -DDAWN_USE_GLFW=OFF \
    -DTINT_BUILD_TESTS=OFF \
    -DTINT_BUILD_CMD_TOOLS=OFF \
    -DTINT_BUILD_WGSL_READER=ON \
    -DTINT_BUILD_WGSL_WRITER=ON \
    -DTINT_BUILD_GLSL_WRITER=ON \
    -DTINT_BUILD_GLSL_VALIDATOR=OFF \
    -DTINT_BUILD_SPV_READER=OFF \
    -DTINT_BUILD_SPV_WRITER=OFF \
    -DTINT_BUILD_HLSL_WRITER=OFF \
    -DTINT_BUILD_MSL_WRITER=OFF \
    -DTINT_BUILD_NULL_WRITER=OFF \
    -DDAWN_BUILD_MONOLITHIC_LIBRARY=STATIC

# Fail before compilation if cache reuse or upstream configuration omitted a
# required writer. Store these actual cache values in the successful receipt.
tint_config="$(python3 - "$DAWN_BUILD_DIR/CMakeCache.txt" <<'PY'
from pathlib import Path
import sys

cache = {}
for line in Path(sys.argv[1]).read_text().splitlines():
    if line.startswith(("//", "#")) or ":" not in line or "=" not in line:
        continue
    key, value = line.split("=", 1)
    name, kind = key.split(":", 1)
    if name in cache:
        raise SystemExit(f"error: duplicate Dawn CMake cache key: {name}")
    cache[name] = (kind, value)
required = {
    "TINT_BUILD_WGSL_READER": "ON",
    "TINT_BUILD_WGSL_WRITER": "ON",
    "TINT_BUILD_GLSL_WRITER": "ON",
    "TINT_BUILD_GLSL_VALIDATOR": "OFF",
    "TINT_BUILD_SPV_READER": "OFF",
    "TINT_BUILD_SPV_WRITER": "OFF",
    "TINT_BUILD_HLSL_WRITER": "OFF",
    "TINT_BUILD_MSL_WRITER": "OFF",
    "TINT_BUILD_NULL_WRITER": "OFF",
}
for key, value in required.items():
    if cache.get(key) != ("BOOL", value):
        raise SystemExit(f"error: required Deko3D Tint option {key}:BOOL={value}, got {cache.get(key)}")
    print(f"dawn_{key.lower()}={value}")
PY
)"
readonly tint_config

echo "[5/6] Building Dawn WebGPU library with native Deko3D backend..."
cmake --build "$DAWN_BUILD_DIR" --target webgpu_dawn --parallel "$JOBS"
readonly DAWN_LIB="$DAWN_BUILD_DIR/src/dawn/native/libwebgpu_dawn.a"
if [[ ! -f "$DAWN_LIB" ]]; then
    echo "error: Dawn build did not produce $DAWN_LIB" >&2
    exit 5
fi

if [[ -x "$DEVKITPRO/devkitA64/bin/aarch64-none-elf-nm" ]]; then
    nm_scan="$(mktemp)"
    trap 'rm -f -- "${nm_scan:-}"' EXIT
    "$DEVKITPRO/devkitA64/bin/aarch64-none-elf-nm" -C "$DAWN_LIB" > "$nm_scan"
    if ! grep -Fq 'dawn::native::deko3d' "$nm_scan"; then
        echo "error: libwebgpu_dawn.a does not expose Deko3D backend symbols" >&2
        exit 5
    fi
    rm -f -- "$nm_scan"
    trap - EXIT
fi

echo "[6/6] Writing reproducible dependency receipt..."
uam_diff="$(git -C "$UAM_DIR" diff HEAD --binary | sha256sum | awk '{print $1}')"
dawn_diff="$(git -C "$DAWN_DIR" diff HEAD --binary -- . ':(exclude)third_party/abseil-cpp' | sha256sum | awk '{print $1}')"
dawn_abseil_diff="$(git -C "$DAWN_DIR/third_party/abseil-cpp" diff HEAD --binary | sha256sum | awk '{print $1}')"
{
    echo "driftdroid_reference=$REF_PIN"
    echo "dawn_repo=$DAWN_REPO"
    echo "dawn_base_commit=$DAWN_PIN"
    echo "dawn_patch_sha256=$(sha256sum "$DAWN_PATCH" | awk '{print $1}')"
    echo "dawn_patch_excluded_gitlinks=$DAWN_EXCLUDE_EGL,$DAWN_EXCLUDE_ABSEIL"
    echo "dawn_switch_portability_file=$DAWN_SWITCH_PORTABILITY_FILE"
    echo "dawn_switch_sleep_file=$DAWN_SWITCH_SLEEP_FILE"
    echo "dawn_switch_message_file=$DAWN_SWITCH_MESSAGE_FILE"
    echo "dawn_switch_patcher_sha256=$(sha256sum "$DAWN_SWITCH_PATCHER" | awk '{print $1}')"
    echo "dawn_worktree_diff_sha256=$dawn_diff"
    echo "dawn_archive_sha256=$(sha256sum "$DAWN_LIB" | awk '{print $1}')"
    echo "$tint_config"
    echo "dawn_cmake_cache_sha256=$(sha256sum "$DAWN_BUILD_DIR/CMakeCache.txt" | awk '{print $1}')"
    echo "dawn_abseil_commit=$DAWN_ABSEIL_PIN"
    echo "dawn_abseil_patcher_sha256=$(sha256sum "$DAWN_ABSEIL_PATCHER" | awk '{print $1}')"
    echo "dawn_abseil_diff_sha256=$dawn_abseil_diff"
    echo "aurora_abseil_patch_sha256=$(sha256sum "$AURORA_ABSEIL_PATCH" | awk '{print $1}')"
    echo "uam_repo=$UAM_REPO"
    echo "uam_commit=$UAM_PIN"
    echo "uam_worktree_diff_sha256=$uam_diff"
    echo "uam_archive_sha256=$(sha256sum "$UAM_LIB" | awk '{print $1}')"
    echo "uam_cross_file_sha256=$(sha256sum "$CROSS_FILE" | awk '{print $1}')"
    echo "devkitpro=$DEVKITPRO"
    echo "compiler=$($DEVKITPRO/devkitA64/bin/aarch64-none-elf-g++ --version | head -n 1)"
    echo "cmake=$(cmake --version | head -n 1)"
    echo "ninja=$(ninja --version)"
    echo "meson=$(meson --version)"
} > "$RECEIPT"

cat <<EOF_DONE
Native Deko3D dependencies are ready.
  Dawn source : $DAWN_DIR @ $DAWN_PIN
  Dawn build  : $DAWN_BUILD_DIR
  Dawn lib    : $DAWN_LIB
  UAM source  : $UAM_DIR @ $UAM_PIN
  UAM lib     : $UAM_LIB
  receipt     : $RECEIPT

Next command:
  scripts/build-local-native-deko3d.sh
EOF_DONE
