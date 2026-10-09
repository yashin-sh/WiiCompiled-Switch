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
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXTransform.cpp").read_text()
signature = "void GXLoadNrmMtxImm(const void* mtx_, u32 id)"
(test / "pinned-normal-native.inc").write_text(signature + " {" + source.split(signature + " {", 1)[1].split("\n}", 1)[0] + "\n}\n")
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/GXTev.cpp").read_text()
signature = "void GXSetZTexture(GXZTexOp op, GXTexFmt fmt, u32 bias)"
(test / "pinned-z-texture-native.inc").write_text(signature + " {" + source.split(signature + " {", 1)[1].split("\n}", 1)[0] + "\n}\n")
signature = "void GXSetTevColorS10(GXTevRegID id, GXColorS10 color)"
(test / "pinned-tev-s10-native.inc").write_text(signature + " {" + source.split(signature + " {", 1)[1].split("\n}", 1)[0] + "\n}\n")
source = (root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx/__gx.h").read_text()
(test / "pinned-gx-data.inc").write_text("struct __GXData_struct {" + source.split("struct __GXData_struct {", 1)[1].split("\n};", 1)[0] + "\n};\n")
(test / "pinned-set-reg-field.inc").write_text("#define SET_REG_FIELD" + source.split("#define SET_REG_FIELD", 1)[1].split("\n\n", 1)[0] + "\n")
source = (root / "third_party/WiiCompiled/aurora-main/lib/gx/command_processor.cpp").read_text()
cases = ["case 0xF4: {", "case 0xF5: {"]
(test / "pinned-z-texture-decode.inc").write_text("\n".join(case + source.split(case, 1)[1].split("\n  }", 1)[0] + "\n}" for case in cases))
start = source.index("  case 0xE0:\n")
end = source.index("  // Indirect texture matrices", start)
(test / "pinned-tev-s10-decode.inc").write_text(source[start:end])
for signature, name in [("inline static u32 bp_get(u32 reg, u32 size, u32 shift)", "pinned-bp-get.inc"),
                        ("static inline void mark_pipeline_state_dirty() noexcept", "pinned-pipeline-dirty.inc")]:
    suffix = source.split(signature + " {", 1)[1]
    body = suffix.split("}", 1)[0] if name == "pinned-bp-get.inc" else suffix.split("\n}", 1)[0]
    (test / name).write_text(signature + " {" + body + "}\n")

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
    for contract in gx_light_object gx_normal_matrix gx_z_texture gx_tev_color_s10; do
        "$HOST_CXX" "${flags[@]}" -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
            "$ROOT_DIR/tests/${contract}_contract.cpp" "$ROOT_DIR/source/memory_switch_slice.cpp" \
            "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" "$ROOT_DIR/source/gx_load_light_obj_hle_bridge.cpp" \
            "$ROOT_DIR/source/gx_load_nrm_mtx_imm_hle_bridge.cpp" "$ROOT_DIR/source/gx_set_z_texture_hle_bridge.cpp" \
            "$ROOT_DIR/source/gx_set_tev_color_s10_hle_bridge.cpp" \
            "$ROOT_DIR/tests/lyt_quad_unexercised_fixture.cpp" -o "$TEST_DIR/$contract-$rendered"
        "$TEST_DIR/$contract-$rendered"
    done
done
"$HOST_CXX" "${flags[@]}" "$ROOT_DIR/tests/missing_native_extension_priority_contract.cpp" -o "$TEST_DIR/priority"
"$TEST_DIR/priority"

# Compile the probe implementation independently of all translated headers/flags.
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror "-fsanitize=address,undefined" \
    -c "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" -o "$TEST_DIR/probe.o"
"$HOST_CXX" "${flags[@]}" "$ROOT_DIR/tests/missing_native_extension_probe_contract.cpp" \
    "$TEST_DIR/probe.o" -o "$TEST_DIR/probe"
"$TEST_DIR/probe"
