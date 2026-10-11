#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
mkdir -p "$TEST_DIR/sdmc:/switch/WiiCompiled-Switch"

# Extract authoritative public guest packing/LOD and native size oracles from
# the pinned source, never from generated game code or duplicated test packing.
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
from pathlib import Path
import subprocess
import sys
root, test = map(Path, sys.argv[1:])
wii = root / 'third_party/WiiCompiled'
pin = 'a135beb201042b20f390c6695ca6b26768820fb4'
assert subprocess.check_output(['git', '-C', str(wii), 'rev-parse', 'HEAD']).decode().strip() == pin
def read(path):
    return subprocess.check_output(['git', '-C', str(wii), 'show', pin + ':' + path]).decode('utf-8-sig')
def function(text, signature):
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]
guest = read('runtime/src/hle/gx/gx_texture.cpp')
native = read('aurora-main/lib/dolphin/gx/GXTexture.cpp')
canonical = function(read('runtime/include/gx_guest_write.h'), 'inline uint32_t CanonicalizeGxMainRamAddress(')
pack = function(guest, 'static void WriteGuestTexObj(')
lod = function(guest, 'static void WriteGuestTexObjLOD(')
table = next(line for line in guest.splitlines() if line.startswith('static const uint8_t kGxToHwMinFilter['))
size = function(native, 'u32 GXGetTexBufferSize(')
body = '\n'.join([canonical, table, pack, lod, size])
for old, new in [('CanonicalizeGxMainRamAddress', 'PinnedCanonicalizeGxMainRamAddress'),
                 ('WriteGuestTexObjLOD', 'PinnedWriteGuestTexObjLOD'),
                 ('WriteGuestTexObj', 'PinnedWriteGuestTexObj'),
                 ('GXGetTexBufferSize', 'PinnedGXGetTexBufferSize')]:
    # Token replacement avoids touching the already renamed LOD identifier.
    import re
    body = re.sub(r'\b' + old + r'\b', new, body)
(test / 'pinned-rgb565-guest-texture.inc').write_text(body + '\n')
PY

# The existing large texture bridge has two legacy unused observed constants.
# Keep all other warnings fatal while compiling that real translation unit.
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-const-variable \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/ci-rendered-compile-seams" \
        -I"$ROOT_DIR/local-rendered-fast-track/seams" \
        -I"$ROOT_DIR/include" \
        -I"$TEST_DIR" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_texture_load_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_init_tex_obj_hle_bridge.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    (cd "$TEST_DIR" && ./"contract-$rendered")
    echo "PASS: GX texture load contract (rendered=$rendered)"
done

# LOD shares the real texture bridge and runs in every existing texture CI gate.
bash "$ROOT_DIR/scripts/test-gx-depth-lod.sh"
