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
readonly ABSEIL_PATCH="$REF_DIR/aurora-main/cmake/patches/abseil-nintendo-switch-platform.patch"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

for tool in git cmake ninja meson python3 bison flex sha256sum; do
    need "$tool"
done

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

    # A --no-checkout clone has a valid HEAD but intentionally no worktree, so
    # `git diff --quiet` reports every tracked path as deleted. Treat only that
    # empty worktree as fresh. Any existing files must still pass the normal
    # dirty-worktree guard before we move the checkout.
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

apply_exact_patch() {
    local checkout="$1" patch="$2" name="$3"
    if git -C "$checkout" apply --reverse --check "$patch" >/dev/null 2>&1; then
        return 0
    fi
    if ! git -C "$checkout" apply --check "$patch"; then
        echo "error: $name patch does not apply cleanly to pinned source" >&2
        exit 4
    fi
    git -C "$checkout" apply "$patch"
}

assert_only_patch_changes() {
    local checkout="$1" patch="$2" name="$3" extra_allowed="${4:-}"
    local changed path allowed
    changed="$( { git -C "$checkout" diff --name-only HEAD; git -C "$checkout" ls-files --others --exclude-standard; } | sort -u)"
    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        if [[ -n "$extra_allowed" && "$path" == "$extra_allowed" ]]; then
            continue
        fi
        allowed=0
        if grep -Eq "^(--- a/|\+\+\+ b/)${path//./\.}$" "$patch"; then
            allowed=1
        fi
        if ((allowed == 0)); then
            echo "error: unexpected local change in $name checkout: $path" >&2
            exit 4
        fi
    done <<< "$changed"
}

echo "[1/6] Preparing static UAM library source..."
ensure_checkout "$UAM_REPO" "$UAM_DIR" "$UAM_PIN" "libuam"

# This cross file is the public Switch recipe used by projects packaging averne/libuam,
# made path-independent by resolving DEVKITPRO here.
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

# switchvars provides the libnx/portlibs compile and link environment expected by
# the public libuam Switch package recipe.
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
if [[ -e "$DAWN_DIR/.git" ]] && [[ "$(git -C "$DAWN_DIR" rev-parse HEAD 2>/dev/null || true)" == "$DAWN_PIN" ]] && \
   git -C "$DAWN_DIR" apply --reverse --check "$DAWN_PATCH" >/dev/null 2>&1; then
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

apply_exact_patch "$DAWN_DIR" "$DAWN_PATCH" "Dawn Deko3D"
apply_exact_patch "$DAWN_DIR/third_party/abseil-cpp" "$ABSEIL_PATCH" "Dawn Abseil Switch"

# Require exactly our two authorized source deltas: the Dawn backend patch and
# the Abseil newlib/libnx portability patch. Their hashes are recorded below.
if ! git -C "$DAWN_DIR" apply --reverse --check "$DAWN_PATCH" >/dev/null 2>&1; then
    echo "error: Dawn Deko3D patch verification failed after apply" >&2
    exit 4
fi
if ! git -C "$DAWN_DIR/third_party/abseil-cpp" apply --reverse --check "$ABSEIL_PATCH" >/dev/null 2>&1; then
    echo "error: Abseil Switch patch verification failed after apply" >&2
    exit 4
fi
assert_only_patch_changes "$DAWN_DIR" "$DAWN_PATCH" "Dawn" "third_party/abseil-cpp"
assert_only_patch_changes "$DAWN_DIR/third_party/abseil-cpp" "$ABSEIL_PATCH" "Dawn Abseil"

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
    -DDAWN_BUILD_MONOLITHIC_LIBRARY=STATIC

echo "[5/6] Building Dawn WebGPU library with native Deko3D backend..."
cmake --build "$DAWN_BUILD_DIR" --target webgpu_dawn --parallel "$JOBS"
readonly DAWN_LIB="$DAWN_BUILD_DIR/src/dawn/native/libwebgpu_dawn.a"
if [[ ! -f "$DAWN_LIB" ]]; then
    echo "error: Dawn build did not produce $DAWN_LIB" >&2
    exit 5
fi

# Confirm the backend made it into the archive before a much more expensive game build.
# Do not pipe nm directly into grep -q under pipefail: grep can exit after the first match,
# SIGPIPE nm and turn a successful symbol lookup into a false negative.
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
abseil_diff="$(git -C "$DAWN_DIR/third_party/abseil-cpp" diff HEAD --binary | sha256sum | awk '{print $1}')"
{
    echo "driftdroid_reference=$REF_PIN"
    echo "dawn_repo=$DAWN_REPO"
    echo "dawn_base_commit=$DAWN_PIN"
    echo "dawn_patch_sha256=$(sha256sum "$DAWN_PATCH" | awk '{print $1}')"
    echo "dawn_worktree_diff_sha256=$dawn_diff"
    echo "dawn_archive_sha256=$(sha256sum "$DAWN_LIB" | awk '{print $1}')"
    echo "abseil_patch_sha256=$(sha256sum "$ABSEIL_PATCH" | awk '{print $1}')"
    echo "abseil_worktree_diff_sha256=$abseil_diff"
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
