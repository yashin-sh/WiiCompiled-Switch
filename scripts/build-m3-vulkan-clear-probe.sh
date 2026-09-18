#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly MESA_PIN="b297e230ef88c6c88df2561becf864f979f494a6"
readonly MESA_REPO="https://github.com/danfromtico/mesa-switch.git"
readonly DEPS_DIR="${MKW_M3_DEPS_DIR:-$ROOT_DIR/.deps/m3}"
readonly MESA_DIR="${MKW_M3_MESA_ROOT:-$DEPS_DIR/mesa-switch}"
readonly MESA_PATCH="$ROOT_DIR/patches/mesa-switch/m3-linux-build.patch"
readonly MESA_IMAGE="${MKW_M3_MESA_IMAGE:-wiicompiled-m3-mesa-b297e230-v3}"
readonly RUST_TARGET="aarch64-unknown-linux-gnu"
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

if [[ ! -f "$MESA_PATCH" ]]; then
    echo "error: missing pinned mesa-switch integration patch: $MESA_PATCH" >&2
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

mkdir -p "$DEPS_DIR"

if [[ ! -d "$MESA_DIR/.git" ]]; then
    echo "[1/5] Cloning pinned mesa-switch dependency..."
    git clone --filter=blob:none "$MESA_REPO" "$MESA_DIR"
else
    echo "[1/5] Reusing mesa-switch checkout..."
fi

git -C "$MESA_DIR" fetch --quiet origin "$MESA_PIN"
git -C "$MESA_DIR" checkout --quiet --detach "$MESA_PIN"

actual_pin="$(git -C "$MESA_DIR" rev-parse HEAD)"
if [[ "$actual_pin" != "$MESA_PIN" ]]; then
    echo "error: mesa-switch pin mismatch: $actual_pin" >&2
    exit 1
fi
echo "      mesa-switch: $actual_pin"

# Keep the upstream pin exact and re-apply only our auditable build-integration
# delta. Build directories and other untracked local output are preserved.
git -C "$MESA_DIR" restore --source="$MESA_PIN" -- Docker.rust build-switch.sh
if ! git -C "$MESA_DIR" apply --check "$MESA_PATCH"; then
    echo "error: local mesa-switch integration patch no longer applies to $MESA_PIN" >&2
    exit 1
fi
git -C "$MESA_DIR" apply "$MESA_PATCH"
echo "[2/5] Applied pinned Linux/Rust/SELinux integration patch."

need_mesa_build=0
if [[ ! -f "$VULKAN_ARCHIVE" || "${MKW_M3_FORCE_MESA_REBUILD:-0}" == "1" ]]; then
    need_mesa_build=1
fi
if ! docker image inspect "$MESA_IMAGE" >/dev/null 2>&1; then
    need_mesa_build=1
fi

if ((need_mesa_build != 0)); then
    echo "[3/5] Building loaderless NVK/Mesa for Switch..."
    (
        cd "$MESA_DIR"
        MESA_SWITCH_IMAGE_NAME="$MESA_IMAGE" \
        MESA_SWITCH_RUST_TARGET="$RUST_TARGET" \
        ./build-switch.sh
    )
else
    echo "[3/5] Reusing existing loaderless NVK archive and dedicated image."
fi

if ! docker image inspect "$MESA_IMAGE" >/dev/null 2>&1; then
    echo "error: expected Mesa build image '$MESA_IMAGE' is missing after build" >&2
    exit 1
fi
if [[ ! -f "$VULKAN_ARCHIVE" ]]; then
    echo "error: Mesa build did not produce $VULKAN_ARCHIVE" >&2
    exit 1
fi

echo "[4/5] Building isolated M3 Vulkan clear probe..."
docker run --rm \
    "${DOCKER_SECURITY_ARGS[@]}" \
    -e MKW_M3_JOBS="$JOBS" \
    -e MESA_SWITCH_RUST_TARGET="$RUST_TARGET" \
    -v "$MESA_DIR:/mesa:ro" \
    -v "$ROOT_DIR:/work" \
    -w /work/m3-graphics-probe \
    "$MESA_IMAGE" \
    bash -lc '
        set -euo pipefail
        export DEVKITPRO=/opt/devkitpro

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
            adler
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
            if ((${#matches[@]} != 1)); then
                echo "error: expected exactly one Rust std archive for $stem in $target_libdir; found ${#matches[@]}" >&2
                exit 1
            fi
            rust_libs+=("${matches[0]}")
        done
        rust_std_libs="${rust_libs[*]}"

        echo "Rust target: $MESA_SWITCH_RUST_TARGET"
        echo "Rust target libdir: $target_libdir"
        echo "Rust std closure: ${#rust_libs[@]} archives"

        make -j"$MKW_M3_JOBS" \
            MESA_SWITCH_ROOT=/mesa \
            RUST_STD_LIBS="$rust_std_libs"
    '

if [[ ! -f "$OUTPUT" ]]; then
    echo "error: probe NRO was not produced: $OUTPUT" >&2
    exit 1
fi

echo "[5/5] Probe ready."
echo "NRO: $OUTPUT"
echo
echo "Copy it to:"
echo "  /switch/WiiCompiled-Switch-m3-vulkan-clear-probe/WiiCompiled-Switch-m3-vulkan-clear-probe.nro"
echo
echo "Hardware status:"
echo "  - NVK/VI changing-color clear/present is already validated on real Switch;"
echo "  - use this target for reproducibility/regression checks;"
echo "  - the next #162 feature target is a Vulkan triangle."
