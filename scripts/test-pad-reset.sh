#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
mkdir -p "$TEST_DIR/sdmc:/switch/WiiCompiled-Switch"
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = a135beb201042b20f390c6695ca6b26768820fb4
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--wrap=fopen \
    -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/local-rendered-fast-track/seams" -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    "$ROOT_DIR/tests/pad_reset_contract.cpp" \
    "$ROOT_DIR/source/pad_reset_hle_bridge.cpp" -o "$TEST_DIR/pad-reset-contract"
(cd "$TEST_DIR" && ./pad-reset-contract)
