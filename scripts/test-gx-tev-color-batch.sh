#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

# Actual Switch range checking, allocation-only Horizon seam, forwarding-only
# Aurora sinks. GNU linker wrapping observes ID-before-memory ordering.
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_tev_color_batch_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_set_tev_k_color_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_color_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_swap_mode_table_hle_bridge.cpp" \
        -Wl,--wrap=_ZN6Memory10GetPointerEjm \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
    echo "PASS: GX TEV color batch contract (rendered=$rendered)"
done
