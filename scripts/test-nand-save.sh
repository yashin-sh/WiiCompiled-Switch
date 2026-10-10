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
git -C "$WII_DIR" show "$WII_PIN:runtime/src/hle/storage/nand_api.cpp" > "$TEST_DIR/pinned-source"
cmp "$TEST_DIR/pinned-source" "$WII_DIR/runtime/src/hle/storage/nand_api.cpp"
python3 - "$WII_DIR" "$TEST_DIR" <<'PYTHON'
from pathlib import Path
import sys
root, out = map(Path, sys.argv[1:])
source = (root / 'runtime/src/hle/storage/nand_api.cpp').read_text()
bodies = []
for name, address in [('NANDWrite', '8019B884'), ('NANDCreateDir', '8019BBE0')]:
    start = source.index('extern "C" int32_t ' + name + '_HLE(')
    end = source.index('PPC_NATIVE_OVERRIDE(' + address, start)
    bodies.append(source[start:end])
(out / 'pinned-nand-save.inc').write_text('\n'.join(bodies))
PYTHON
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-parameter \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -Wl,--wrap=fopen -Wl,--wrap=fflush -Wl,--wrap=fsync -Wl,--wrap=fclose \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" -I"$TEST_DIR" \
        -isystem "$WII_DIR/runtime/include" \
        "$ROOT_DIR/tests/nand_save_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/switch_nand_open_runtime.cpp" \
        "$ROOT_DIR/source/switch_nand_callback_runtime.cpp" -o "$TEST_DIR/contract-$rendered"
    mkdir -p "$TEST_DIR/run-$rendered"
    (cd "$TEST_DIR/run-$rendered" && ../"contract-$rendered")
    echo "PASS: NAND save production native traits/Memory/callbacks/filesystem/pinned oracles (rendered=$rendered)"
done
