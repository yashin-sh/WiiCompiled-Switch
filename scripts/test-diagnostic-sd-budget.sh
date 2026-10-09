#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--wrap=fsync \
    -DTARGET_PC -DMKW_LOCAL_FAST_TRACK=1 -DMKW_DISCOVERY_SCAN_MODE=1 \
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
    -I"$ROOT_DIR/tests/diagnostic-seams" -I"$ROOT_DIR/include" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
    "$ROOT_DIR/source/fast_track_crash_diagnostics.cpp" \
    "$ROOT_DIR/tests/diagnostic_sd_budget_contract.cpp" -o "$TEST_DIR/contract"
for scenario in clock missing-clock; do
    mkdir "$TEST_DIR/$scenario"
    if [[ "$scenario" == clock ]]; then
        (cd "$TEST_DIR/$scenario" && ../contract)
    else
        (cd "$TEST_DIR/$scenario" && ../contract missing-clock)
    fi
done
