#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly HOST_CXX="${MKW_HOST_CXX:-clang++}"
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
mkdir -p "$TEST_DIR/sdmc:/switch/WiiCompiled-Switch"
test "$(git -C "$ROOT_DIR/third_party/WiiCompiled" rev-parse HEAD)" = a135beb201042b20f390c6695ca6b26768820fb4
python3 - "$ROOT_DIR" "$TEST_DIR" <<'PY'
import importlib.util
from pathlib import Path
import sys
root, test = map(Path, sys.argv[1:])
spec = importlib.util.spec_from_file_location("checked_fifo", root / "scripts/prepare-checked-aurora-fifo.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
gx = root / "third_party/WiiCompiled/aurora-main/lib"
(test / "gx").mkdir()
(test / "dolphin/gx").mkdir(parents=True)
(test / "gx/fifo.hpp").write_text(module.checked_fifo((gx / "gx/fifo.hpp").read_text()))
(test / "dolphin/gx/__gx.h").write_bytes((gx / "dolphin/gx/__gx.h").read_bytes())
(test / "internal.hpp").write_text('''#pragma once
#include <bit>
#include <cstdint>
#include <cassert>
#define LIKELY
#define CHECK(cond, ...) assert(cond)
template<class T> T bswap(T value) {
    if constexpr (sizeof(T) == 2) return static_cast<T>(__builtin_bswap16(std::bit_cast<uint16_t>(value)));
    if constexpr (sizeof(T) == 4) return std::bit_cast<T>(__builtin_bswap32(std::bit_cast<uint32_t>(value)));
    if constexpr (sizeof(T) == 8) return std::bit_cast<T>(__builtin_bswap64(std::bit_cast<uint64_t>(value)));
}
''')
# The real preparation script must preserve upstream files and reuse mirror
# timestamps. A changed pin must fail rather than apply a fuzzy substitution.
module.prepare(gx.parent, test / "mirror")
mirror = test / "mirror/lib/gx/fifo.hpp"
mtime = mirror.stat().st_mtime_ns
module.prepare(gx.parent, test / "mirror")
assert mirror.stat().st_mtime_ns == mtime
try:
    module.checked_fifo((gx / "gx/fifo.hpp").read_text() + "\n")
except ValueError:
    pass
else:
    raise AssertionError("changed FIFO header accepted")
native = (gx / "dolphin/gx/GXDispList.cpp").read_text()
body = native.split('void GXBeginDisplayList(', 1)[1].split('\nvoid GXCallDisplayList(', 1)[0]
fifo = (gx / "gx/fifo.cpp").read_text()
flush = (gx / "dolphin/gx/GXManage.cpp").read_text().split("void GXFlush() {", 1)[1].split("\nvoid GXPixModeSync", 1)[0]
alpha = (gx / "dolphin/gx/GXTev.cpp").read_text().split("void GXSetAlphaCompare(", 1)[1].split("\nvoid GXSetTevOrder(", 1)[0]
record = fifo.split('void begin_display_list(', 1)[1].split('// How much', 1)[0]
(test / "pinned-display-list.inc").write_text(
    'static __GXData_struct sSavedGXData;\nextern "C" {\nvoid GXBeginDisplayList(' + body + '}\n'
    + 'namespace aurora::gx::fifo {\nvoid begin_display_list(' + record + '}\n'
    + 'extern \"C\" void GXFlush() {' + flush
    + 'extern \"C\" void GXSetAlphaCompare(' + alpha)
PY
for rendered in 0 1; do
    "$HOST_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
        -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--wrap=abort \
        -DTARGET_PC -DAURORA -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
        -DMKW_LOCAL_RENDERED_FAST_TRACK="$rendered" \
        -include "$ROOT_DIR/include/devkita64_gcc_compat.hpp" \
        -I"$TEST_DIR" -I"$ROOT_DIR/ci-rendered-compile-seams" \
        -I"$ROOT_DIR/local-rendered-fast-track/seams" -I"$ROOT_DIR/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/include" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/runtime/src/hle/gx" \
        -isystem "$ROOT_DIR/third_party/WiiCompiled/aurora-main/include" \
        "$ROOT_DIR/tests/gx_display_list_contract.cpp" \
        "$ROOT_DIR/source/memory_switch_slice.cpp" \
        "$ROOT_DIR/source/gx_display_list_hle_bridge.cpp" \
        "$ROOT_DIR/source/gx_set_alpha_compare_hle_bridge.cpp" \
        "$ROOT_DIR/m3-aurora-gx-probe/source/display_list_overflow.cpp" \
        "$ROOT_DIR/m3-aurora-gx-probe/source/display_list_transport.cpp" \
        -o "$TEST_DIR/contract-$rendered"
    (cd "$TEST_DIR" && ./"contract-$rendered")
done
