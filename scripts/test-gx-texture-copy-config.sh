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
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/ci-rendered-compile-seams" -I"$ROOT_DIR/local-rendered-fast-track/seams" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_texture_copy_config_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_texture_copy_config_hle_bridge.cpp" \
        -Wl,--wrap=_ZN6Memory10GetPointerEjm \
        -Wl,--wrap=_ZN6Memory6Read32Ej -Wl,--wrap=_ZN6Memory7Write32Ejj -Wl,--wrap=abort \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done

# Execute the three pinned native state setters verbatim, with only storage
# replaced. No framebuffer, renderer, allocation or command transport is used.
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXFrameBuffer.cpp").read_text()
def function(start, end):
    return start + source.split(start, 1)[1].split(end, 1)[0]
(test / "pinned-texture-copy.inc").write_text(
    'extern "C" {\n'
    + function("void GXSetTexCopySrc(", "\nvoid GXSetDispCopyDst(")
    + function("void GXSetTexCopyDst(", "\nvoid GXSetDispCopyFrame2Field(")
    + function("void GXSetCopyClamp(", "\nu32 GXSetDispCopyYScale(") + "}\n"
)
PY
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -I"$TEST_DIR" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/gx_texture_copy_native_contract.cpp" -o "$TEST_DIR/native-contract"
"$TEST_DIR/native-contract"
