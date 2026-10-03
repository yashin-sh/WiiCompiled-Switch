#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

# Real bridges and Aurora signatures; forwarding sinks observe the CPU ABI.
# No guest-memory, frame/FIFO or GPU implementation is supplied at this link.
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_tev_scalar_batch_contract.cpp" \
        "$ROOT_DIR/source/gx_set_tev_direct_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_color_in_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_color_op_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_alpha_in_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_alpha_op_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_swap_mode_hle_bridge.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
    echo "PASS: GX TEV scalar batch contract (rendered=$rendered)"
done
