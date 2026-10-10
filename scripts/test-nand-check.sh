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
for path in runtime/src/hle/storage/nand_api.cpp runtime/src/hle/storage/nand_check_contract.h; do
    git -C "$WII_DIR" show "$WII_PIN:$path" > "$TEST_DIR/pinned-source"
    cmp "$TEST_DIR/pinned-source" "$WII_DIR/$path"
done

for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -Wl,--wrap=_ZN6Memory7Write32Ejj \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" \
        -isystem "$WII_DIR/runtime/include" \
        -isystem "$WII_DIR/runtime/src/hle/storage" \
        "$ROOT_DIR/tests/nand_check_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
    echo "PASS: NANDCheck production trait/Memory/pinned oracle (rendered=$rendered)"
done
