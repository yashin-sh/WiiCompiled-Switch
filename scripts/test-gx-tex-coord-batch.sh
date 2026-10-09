#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = a135beb201042b20f390c6695ca6b26768820fb4

# Execute the actual pinned Gen2 writer and its matrix-index helper. The
# independent oracle checks complete XF/CP bytes and untouched state fields.
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PYTHON'
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
native = root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx"
parts = []
for name, signature in [("GXManage.cpp", "void __GXSetMatrixIndex(GXAttr matIdxAttr)"),
                        ("GXGeometry.cpp", "void GXSetTexCoordGen2(GXTexCoordID dst, GXTexGenType type, GXTexGenSrc src, u32 mtx, GXBool normalize, u32 postMtx)")]:
    text = (native / name).read_text()
    parts.append(signature + " {" + text.split(signature + " {", 1)[1].split("\n}", 1)[0] + "\n}")
text = (native / "__gx.h").read_text()
parts.insert(0, "struct __GXData_struct {" + text.split("struct __GXData_struct {", 1)[1].split("\n};", 1)[0] + "\n};\n__GXData_struct state{};\n__GXData_struct* __gx = &state;")
for name in ("SET_REG_FIELD", "GX_WRITE_XF_REG", "GX_WRITE_SOME_REG4"):
    macro = "#define " + text.split("#define " + name, 1)[1]
    parts.insert(0, "#define " + name + macro.removeprefix("#define ").split("\n\n", 1)[0])
(test / "pinned-gen2.inc").write_text("\n".join(parts).replace("void GXSetTexCoordGen2(", "void PinnedGXSetTexCoordGen2("))
PYTHON

for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_tex_coord_batch_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_set_tex_coord_scale_manually_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tex_coord_bias_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tex_coord_gen2_hle_bridge.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
    echo "PASS: GX texture coordinate batch contract (rendered=$rendered)"
done

"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -I"$TEST_DIR" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/gx_tex_coord_gen2_native_contract.cpp" -o "$TEST_DIR/native"
"$TEST_DIR/native"
