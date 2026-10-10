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
for path in runtime/src/hle/storage/nand_api.cpp runtime/src/hle/storage/nand_fs.cpp; do
    git -C "$WII_DIR" show "$WII_PIN:$path" > "$TEST_DIR/pinned-source"
    cmp "$TEST_DIR/pinned-source" "$WII_DIR/$path"
done
python3 - "$WII_DIR" "$TEST_DIR" <<'PYTHON'
from pathlib import Path
import sys
root, out = map(Path, sys.argv[1:])
source = (root / 'runtime/src/hle/storage/nand_api.cpp').read_text()
start = source.index('extern "C" int32_t NANDCreate_HLE(')
end = source.index('PPC_NATIVE_OVERRIDE(8019B43C', start)
(out / 'pinned-nand-create.inc').write_text(source[start:end])
PYTHON
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-parameter \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -Wl,--wrap=open -Wl,--wrap=close \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" -I"$TEST_DIR" \
        -isystem "$WII_DIR/runtime/include" \
        "$ROOT_DIR/tests/nand_create_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" \
        "$ROOT_DIR/source/nand_create_hle_bridge.cpp" -o "$TEST_DIR/contract-$rendered"
    mkdir -p "$TEST_DIR/run-$rendered/sdmc:/switch/WiiCompiled-Switch"
    (cd "$TEST_DIR/run-$rendered" && ../"contract-$rendered")
    echo "PASS: scoped NANDCreate production dispatch/Memory/filesystem/pinned oracle (rendered=$rendered)"
done
