#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

# Actual Switch range checking, allocation-only Horizon seam, forwarding-only
# Aurora sinks. GNU linker wrapping observes tuple-before-memory ordering and real aborts.
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_fog_z_comp_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_set_fog_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_z_comp_loc_hle_bridge.cpp" \
        -Wl,--wrap=_ZN6Memory10GetPointerEjm -Wl,--wrap=abort \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
    echo "PASS: GX Fog/ZComp contract (rendered=$rendered)"
done

# Execute the pinned coefficient/register implementation with only transport
# and state-storage seams replaced. Avoid copying a native implementation into
# the repository or testing a second hand-written version of its arithmetic.
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = "$WII_PIN"
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
from pathlib import Path
import sys

root, test = map(Path, sys.argv[1:])
gx = root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx"
pixel = (gx / "GXPixel.cpp").read_text()
header = (gx / "__gx.h").read_text()
macro = header.split("#define SET_REG_FIELD(", 1)[1].split("\n\n", 1)[0]
fog = pixel.split("void GXSetFog(", 1)[1].split("\nvoid GXSetFogColor(", 1)[0]
zcomp = pixel.split("void GXSetZCompLoc(", 1)[1].split("\nvoid GXSetPixelFmt(", 1)[0]
(test / "pinned-gx-fog-pixel.inc").write_text(
    "#define SET_REG_FIELD(" + macro + '\nextern "C" {\n'
    + "void GXSetFog(" + fog + "void GXSetZCompLoc(" + zcomp + "}\n"
)
PY
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fno-fast-math -ffp-contract=off \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DTARGET_PC -I"$TEST_DIR" \
    -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
    "$ROOT_DIR/tests/gx_fog_native_contract.cpp" -o "$TEST_DIR/native-contract"
"$TEST_DIR/native-contract"
