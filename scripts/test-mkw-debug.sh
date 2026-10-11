#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -pthread -fsanitize=address,undefined -fno-sanitize-recover=all -Wl,--wrap=rename \
    -I"$ROOT_DIR/include" "$ROOT_DIR/source/mkw_debug_server.cpp" \
    "$ROOT_DIR/tests/mkw_debug_host.cpp" -o "$TEST_DIR/mkw-debug-host"
cd "$ROOT_DIR"
MKW_DEBUG_HOST_TEST_BINARY="$TEST_DIR/mkw-debug-host" \
    python3 -m unittest discover -s tests -p 'test_mkw_debug.py' -v
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--wrap=fsync \
    -D__SWITCH__ -DTARGET_PC -DMKW_LOCAL_FAST_TRACK=1 -DMKW_DISCOVERY_SCAN_MODE=1 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/tests/diagnostic-seams" -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    "$ROOT_DIR/source/fast_track_crash_diagnostics.cpp" \
    "$ROOT_DIR/tests/mkw_debug_frontier_contract.cpp" -o "$TEST_DIR/frontier-contract"
for scenario in enabled disabled disconnect; do
    mkdir "$TEST_DIR/$scenario"
    (cd "$TEST_DIR/$scenario" && ../frontier-contract "$scenario")
done
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -D__SWITCH__ -I"$ROOT_DIR/tests/debug-reload-seams" -I"$ROOT_DIR/include" \
    "$ROOT_DIR/tests/mkw_debug_reload_contract.cpp" -o "$TEST_DIR/reload-contract"
mkdir "$TEST_DIR/reload-config"
(cd "$TEST_DIR/reload-config" && ../reload-contract)
