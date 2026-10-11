#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly WII_PIN="a135beb201042b20f390c6695ca6b26768820fb4"
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = "$WII_PIN"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
from pathlib import Path
import sys
root, test=map(Path,sys.argv[1:])
gx=root/'third_party/WiiCompiled/aurora-main/lib/dolphin/gx'
header=(gx/'__gx.h').read_text()
macro=header.split('#define SET_REG_FIELD(',1)[1].split('\n\n',1)[0]
source=(gx/'GXTev.cpp').read_text()
blocks=[]
for name,enum in [('Color','GXTevKColorSel'),('Alpha','GXTevKAlphaSel')]:
    signature=f'void GXSetTevK{name}Sel(GXTevStageID id, {enum} sel)'
    blocks.append(signature+' {'+source.split(signature+' {',1)[1].split('\n}',1)[0]+'\n}\n')
(test/'pinned-tev-ksel.inc').write_text('#define SET_REG_FIELD('+macro+'\nextern "C" {\n'+''.join(blocks)+'}\n')
processor=(root/'third_party/WiiCompiled/aurora-main/lib/gx/command_processor.cpp').read_text()
getter=next(line for line in processor.splitlines() if line.startswith('inline static u32 bp_get('))
(test/'pinned-ksel-bp-get.inc').write_text(getter+'\n')
start=processor.index('  case 0xF6:', processor.index('// TEV K color/alpha select'))
end=processor.index('\n  // Fog A/B parameters',start)
(test/'pinned-ksel-decode.inc').write_text(processor[start:end])

PY
for rendered in 0 1; do
    "${MKW_HOST_CXX:-clang++}" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/include" -I"$TEST_DIR" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_tev_ksel_contract.cpp" \
        "$ROOT_DIR/source/gx_set_tev_k_color_sel_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_tev_k_alpha_sel_hle_bridge.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done
