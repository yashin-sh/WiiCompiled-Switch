#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT

test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = a135beb201042b20f390c6695ca6b26768820fb4
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$ROOT_DIR/ci-rendered-compile-seams" -I"$ROOT_DIR/local-rendered-fast-track/seams" \
        -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_pix_mode_sync_contract.cpp" "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_pix_mode_sync_hle_bridge.cpp" \
        -Wl,--wrap=_ZN6Memory6Read32Ej -Wl,--wrap=_ZN6Memory7Write16Ejt -Wl,--wrap=abort \
        -o "$TEST_DIR/contract-$rendered"
    "$TEST_DIR/contract-$rendered"
done

python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
gx = root / "third_party/WiiCompiled/aurora-main/lib/dolphin/gx"
source = (gx / "GXManage.cpp").read_text()
header = (gx / "__gx.h").read_text()
body = "void GXPixModeSync() {" + source.split("void GXPixModeSync() {", 1)[1].split("\n}", 1)[0] + "\n}\n"
macro = "#define GX_WRITE_RAS_REG(value)" + header.split("#define GX_WRITE_RAS_REG(value)", 1)[1].split("\n\n", 1)[0] + "\n"
(test / "pinned-pix-mode-sync.inc").write_text(macro + body)
PY
"$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -I"$TEST_DIR" "$ROOT_DIR/tests/gx_pix_mode_sync_native_contract.cpp" -o "$TEST_DIR/native-contract"
"$TEST_DIR/native-contract"
