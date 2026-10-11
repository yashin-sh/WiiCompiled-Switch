#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-const-variable \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -DTARGET_PC -DMKW_LOCAL_FAST_TRACK=1 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/tests/execution-provider-seams" -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    "$ROOT_DIR/local-execution-support/local_first_translated_execution.cpp" \
    "$ROOT_DIR/source/translated_execution_handoff.cpp" \
    "$ROOT_DIR/tests/execution_cancel_provider_contract.cpp" \
    -o "$TEST_DIR/cancel-provider"
(cd "$TEST_DIR" && ./cancel-provider)
