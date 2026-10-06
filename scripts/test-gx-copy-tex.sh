#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = a135beb201042b20f390c6695ca6b26768820fb4
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/ci-rendered-compile-seams" -I"$ROOT_DIR/local-rendered-fast-track/seams" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_copy_tex_contract.cpp" "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_copy_tex_hle_bridge.cpp" -Wl,--wrap=abort -Wl,--wrap=fopen \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done

# Execute the pinned size calculator verbatim, with only its CHECK sink replaced.
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PYCODE'
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXTexture.cpp").read_text()
function = "u32 GXGetTexBufferSize(" + source.split("u32 GXGetTexBufferSize(", 1)[1].split("\nvoid GXInitTlutObj(", 1)[0]
(test / "pinned-copy-size.inc").write_text('extern "C" {\n' + function + "\n}\n")
PYCODE
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -I"$TEST_DIR" -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/gx_copy_tex_size_contract.cpp" -o "$TEST_DIR/size-contract"
"$TEST_DIR/size-contract"
