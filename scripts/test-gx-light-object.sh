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
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXLighting.cpp").read_text()
signatures = ["void GXInitLightAttn(GXLightObj* light_, float a0, float a1, float a2, float k0, float k1, float k2)",
              "void GXInitLightPos(GXLightObj* light_, float x, float y, float z)",
              "void GXInitLightColor(GXLightObj* light_, GXColor col)",
              "void GXInitLightDir(GXLightObj* light_, float nx, float ny, float nz)",
              "void GXLoadLightObjImm(GXLightObj* light_, GXLightID id)"]
(test / "pinned-light-native.inc").write_text("\n".join(s + " {" + source.split(s + " {", 1)[1].split("\n}", 1)[0] + "\n}" for s in signatures))
source = (root / "third_party/WiiCompiled/aurora-main/lib/gx/gx.hpp").read_text()
(test / "pinned-light-object.inc").write_text("struct GXLightObj_ {" + source.split("struct GXLightObj_ {", 1)[1].split("\n};", 1)[0] + "\n};\n")
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
        "$ROOT_DIR/tests/gx_light_object_contract.cpp" "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" "$ROOT_DIR/source/gx_load_light_obj_hle_bridge.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done
"$HOST_CXX" "${flags[@]}" "$ROOT_DIR/tests/missing_native_extension_priority_contract.cpp" -o "$TEST_DIR/priority"
"$TEST_DIR/priority"

# Compile the probe implementation independently of all translated headers/flags.
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror "-fsanitize=address,undefined" \
    -c "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" -o "$TEST_DIR/probe.o"
"$HOST_CXX" "${flags[@]}" "$ROOT_DIR/tests/missing_native_extension_probe_contract.cpp" \
    "$TEST_DIR/probe.o" -o "$TEST_DIR/probe"
"$TEST_DIR/probe"
