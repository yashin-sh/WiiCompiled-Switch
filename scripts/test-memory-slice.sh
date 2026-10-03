#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -I"$ROOT_DIR/include" \
    "$ROOT_DIR/tests/memory_slice_contract.cpp" \
    "$ROOT_DIR/source/memory_switch_slice.cpp" \
    -o "$TEST_DIR/memory-contract"
"$TEST_DIR/memory-contract"
echo 'PASS: Memory slice range, endian and exception contract'
