#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
readonly WII_DIR="$ROOT_DIR/third_party/WiiCompiled"
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
test "$(git -C "$WII_DIR" rev-parse HEAD)" = "$WII_PIN"
git -C "$WII_DIR" show "$WII_PIN:runtime/include/nand_save_probe.h" > "$TEST_DIR/pinned-source"
cmp "$TEST_DIR/pinned-source" "$WII_DIR/runtime/include/nand_save_probe.h"
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -I"$ROOT_DIR/include" -isystem "$WII_DIR/runtime/include" \
        "$ROOT_DIR/tests/nand_blank_save_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/switch_nand_open_runtime.cpp" -o "$TEST_DIR/contract-$rendered"
    mkdir -p "$TEST_DIR/run-$rendered"
    (cd "$TEST_DIR/run-$rendered" && ../"contract-$rendered")
done
