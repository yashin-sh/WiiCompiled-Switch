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
source = (root / "third_party/WiiCompiled/runtime/src/hle/gx/gx_dl.cpp").read_text()
start = source.index("static inline uint8_t ScaleLytAlpha(")
end = source.index("static bool CanSubmitLytDrawDirect(", start)
suffix = source[source.index("static inline void EmitLytDrawQuad("):]
(test / "pinned-lyt-packet.inc").write_text((source[start:end] + suffix.split("\n}\n", 1)[0] + "\n}\n").replace("Memory::ReadFloat32", "ReadGuestFloat"))
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXDispList.cpp").read_text()
sig = "void GXCallDisplayList(const void* data, u32 nbytes)"
(test / "pinned-call-dl.inc").write_text(sig + " {" + source.split(sig + " {", 1)[1].split("\n}", 1)[0] + "\n}\n")
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/__gx.h").read_text()
(test / "pinned-gx-data.inc").write_text("struct __GXData_struct {" + source.split("struct __GXData_struct {", 1)[1].split("\n};", 1)[0] + "\n};\n")
PYTHON
flags=(-std=c++20 -O2 -Wall -Wextra -Werror "-fsanitize=address,undefined" -fno-sanitize-recover=all -fno-omit-frame-pointer
    -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1
    -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp"
    -I"$ROOT_DIR/ci-rendered-compile-seams" -I"$ROOT_DIR/local-rendered-fast-track/seams"
    -I"$ROOT_DIR/include" -I"$TEST_DIR"
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include"
    -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx"
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include")
for rendered in 0 1; do
    "$HOST_CXX" "${flags[@]}" -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        "$ROOT_DIR/tests/lyt_draw_quad_contract.cpp" "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" \
        "$ROOT_DIR/source/nw4r_lyt_draw_quad_hle_bridge.cpp" -o "$TEST_DIR/lyt-quad-$rendered"
    "$TEST_DIR/lyt-quad-$rendered"
done
