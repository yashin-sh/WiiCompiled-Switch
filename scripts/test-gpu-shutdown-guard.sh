#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -DMKW_LOCAL_RENDERED_FAST_TRACK=1 -I"$ROOT_DIR/include" \
    "$ROOT_DIR/source/mkw_gpu_shutdown_guard.cpp" \
    "$ROOT_DIR/tests/gpu_shutdown_guard_contract.cpp" -o "$TEST_DIR/guard"
"$TEST_DIR/guard" queue
"$TEST_DIR/guard" upload
