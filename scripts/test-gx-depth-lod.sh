#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
mkdir -p "$TEST_DIR/sdmc:/switch/WiiCompiled-Switch"
for rendered in 0 1; do
    for strict in 0 1; do
        "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-const-variable \
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
            -ffunction-sections -fdata-sections -Wl,--gc-sections \
            -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
            -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
            -DMKW_STRICT_GX_TEXTURE_OBSERVED_TUPLES="$strict" \
            -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
            -I"$ROOT_DIR/ci-rendered-compile-seams" \
            -I"$ROOT_DIR/local-rendered-fast-track/seams" -I"$ROOT_DIR/include" \
            -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
            -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
            -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
            "$ROOT_DIR/tests/gx_depth_lod_contract.cpp" \
            "$ROOT_DIR/source/memory_switch_slice.cpp" \
            "$ROOT_DIR/source/gx_init_tex_obj_hle_bridge.cpp" -o "$TEST_DIR/contract-$rendered-$strict"
        (cd "$TEST_DIR" && ./"contract-$rendered-$strict")
    done
done
# Extract the pinned native body rather than duplicate its implementation.
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = "$WII_PIN"
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
gx = root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx"
texture = (gx / "GXTexture.cpp").read_text()
macro = (gx / "__gx.h").read_text().split("#define SET_REG_FIELD(", 1)[1].split("\n\n", 1)[0]
lod = texture.split("void GXInitTexObjLOD(", 1)[1].split("\nvoid GXInitTexObjData(", 1)[0]
table = next(line for line in texture.splitlines() if line.startswith("constexpr u8 GX2HWFiltConv[6]"))
(test / "pinned-gx-depth-lod.inc").write_text(
    "#define SET_REG_FIELD(" + macro + "\n" + table + '\nextern "C" {\nvoid GXInitTexObjLOD(' + lod + "}\n"
)
PY
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -I"$TEST_DIR" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/gx_depth_lod_native_contract.cpp" -o "$TEST_DIR/native-contract"
"$TEST_DIR/native-contract"
