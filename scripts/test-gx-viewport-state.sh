#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = a135beb201042b20f390c6695ca6b26768820fb4
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PYTHON'
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXTransform.cpp").read_text()
body = "void GXSetZScaleOffset(f32 scale, f32 offset) {" + source.split("void GXSetZScaleOffset(f32 scale, f32 offset) {", 1)[1].split("\n}", 1)[0] + "\n}\n"
(test / "pinned-z-scale-offset.inc").write_text(body)
source = (root / "third_party/WiiCompiled/runtime/include/memory_access.h").read_text()
body = "std::uint32_t PinnedSingleBits(double value) {" + source.split("MKW_MEMORY_FORCE_INLINE uint32_t ConvertPpcDoubleToSingleBits(double value) {", 1)[1].split("\n}", 1)[0] + "\n}\n"
(test / "pinned-single-bits.inc").write_text(body)
PYTHON
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/ci-rendered-compile-seams" -I"$ROOT_DIR/local-rendered-fast-track/seams" \
        -I"$ROOT_DIR/include" -I"$TEST_DIR" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_viewport_state_contract.cpp" "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_viewport_state_hle_bridge.cpp" "$ROOT_DIR/source/gx_viewport_hle_bridge.cpp" \
        -Wl,--wrap=abort \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done


"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -I"$TEST_DIR" "$ROOT_DIR/tests/gx_viewport_state_native_contract.cpp" -o "$TEST_DIR/native-contract"
"$TEST_DIR/native-contract"
