#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
for rendered in 0 1; do
    "${MKW_HOST_CXX:-clang++}" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        "$ROOT_DIR/tests/gx_projection_vector_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_projection_hle_bridge.cpp" -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done
