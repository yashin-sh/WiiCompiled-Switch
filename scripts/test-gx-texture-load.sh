#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
mkdir -p "$TEST_DIR/sdmc:/switch/WiiCompiled-Switch"

# The existing large texture bridge has two legacy unused observed constants.
# Keep all other warnings fatal while compiling that real translation unit.
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-const-variable \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/ci-rendered-compile-seams" \
        -I"$ROOT_DIR/local-rendered-fast-track/seams" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_texture_load_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_init_tex_obj_hle_bridge.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    (cd "$TEST_DIR" && ./"contract-$rendered")
    echo "PASS: GX texture load contract (rendered=$rendered)"
done
