#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

# Allocation-only coverage uses the real backend with coroutine entry/switch
# stubs. AArch64 register preservation still requires the Switch probe.
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    "$ROOT_DIR/tests/host_context_contract.cpp" \
    "$ROOT_DIR/source/host_context_switch.cpp" \
    -Wl,--wrap=memalign \
    -o "$TEST_DIR/host-context-contract"
"$TEST_DIR/host-context-contract"
echo 'PASS: HostContext stack allocation and scheduler ownership contract'
