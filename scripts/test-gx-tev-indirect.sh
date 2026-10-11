#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = "$WII_PIN"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
python3 - "$ROOT_DIR" "$TEST_DIR" "$WII_PIN" <<'PY'
from pathlib import Path
import subprocess
import sys
root, test = map(Path,sys.argv[1:3])
pin=sys.argv[3]
wii=root/'third_party/WiiCompiled'
for name in ['runtime/src/hle/gx/gx_indirect.cpp',
             'aurora-main/include/dolphin/gx/GXEnum.h',
             'aurora-main/lib/dolphin/gx/GXBump.cpp','aurora-main/lib/dolphin/gx/__gx.h',
             'aurora-main/lib/gx/command_processor.cpp','aurora-main/lib/gx/gx.hpp','aurora-main/lib/gx/shader.cpp']:
    assert (wii/name).read_bytes()==subprocess.check_output(['git','-C',str(wii),'show',pin+':'+name]), name
# Local SDK patches only shadow the ABI header's Memory include. Require
# the complete actual argument loader to remain identical to the pinned ABI.
abi=(wii/'runtime/include/abi_bridge.h').read_text()
pinned_abi=subprocess.check_output(['git','-C',str(wii),'show',pin+':runtime/include/abi_bridge.h']).decode()
loader=lambda text:text.split('static std::remove_reference_t<T> LoadArgument(',1)[1].split('static void InvokeCpu(TargetFn target, CpuContext* cpu)',1)[0]
assert loader(abi)==loader(pinned_abi)
native=wii/'aurora-main/lib/dolphin/gx'
header=(native/'__gx.h').read_text()
macro=header.split('#define SET_REG_FIELD(',1)[1].split('\n\n',1)[0]
(test/'pinned-indirect-gx-data.inc').write_text('struct __GXData_struct {'+header.split('struct __GXData_struct {',1)[1].split('\n};',1)[0]+'\n};\n')
wrapper=(wii/'runtime/src/hle/gx/gx_indirect.cpp').read_text()
start=wrapper.index('extern "C" void GX__SetTevIndirect_801717ac(')
end=wrapper.index('\n}',start)+2
wrapper=wrapper[start:end]
source=(native/'GXBump.cpp').read_text()
start=source.index('void GXSetTevIndirect(')
end=source.index('\n}',start)+2
(test/'pinned-tev-indirect.inc').write_text('#define SET_REG_FIELD('+macro+'\nextern "C" {\n'+source[start:end]+'\n'+wrapper+'\n}\n')
gx=(wii/'aurora-main/lib/gx/gx.hpp').read_text()
start=gx.index('inline u32 next_gx_state_epoch() noexcept {')
end=gx.index('\n}',start)+2
(test/'pinned-indirect-epoch.inc').write_text(gx[start:end]+'\n')
processor=(wii/'aurora-main/lib/gx/command_processor.cpp').read_text()
start=processor.index('static inline void mark_pipeline_state_dirty() noexcept {')
end=processor.index('\n}',start)+2
(test/'pinned-indirect-pipeline-dirty.inc').write_text(processor[start:end]+'\n')
getter=next(line for line in processor.splitlines() if line.startswith('inline static u32 bp_get('))
(test/'pinned-indirect-bp-get.inc').write_text(getter+'\n')
start=processor.index('  case 0x10:',processor.index('// TEV indirect stages'))
end=processor.index('\n  // Scissor registers',start)
(test/'pinned-indirect-decode.inc').write_text(processor[start:end])
PY
for rendered in 0 1; do
    "${MKW_HOST_CXX:-clang++}" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" -I"$TEST_DIR" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_tev_indirect_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_set_tev_indirect_hle_bridge.cpp" \
        "$ROOT_DIR/source/missing_native_cpu_extensions.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done
