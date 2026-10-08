#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly BUILD_ROOT="${MKW_REPLAY_BUILD_ROOT:-$ROOT_DIR/.deps/desktop-gx-replay}"
readonly OUT_ROOT="$BUILD_ROOT/synthetic-run"
readonly REPLAY="$BUILD_ROOT/mkw-gx-replay"

# Mesa distributions use either lvp_icd.json or an architecture-suffixed name.
# Keep an explicit caller-selected driver; otherwise find the installed lavapipe
# manifest instead of assuming this machine's filename exists on the CI runner.
if [[ -z "${VK_ICD_FILENAMES:-}" && -z "${VK_DRIVER_FILES:-}" ]]; then
    shopt -s nullglob
    replay_icds=(/usr/share/vulkan/icd.d/lvp_icd*.json)
    shopt -u nullglob
    if ((${#replay_icds[@]} != 1)); then
        echo "error: expected one installed lavapipe manifest; set VK_DRIVER_FILES or VK_ICD_FILENAMES explicitly" >&2
        exit 1
    fi
    export VK_ICD_FILENAMES="${replay_icds[0]}"
fi

mkdir -p "$OUT_ROOT"
"$BUILD_ROOT/replay-format-test"
# Separate processes force resource pointer relocation and fresh renderer state.
"$REPLAY" capture "$OUT_ROOT/scene.mkwr" "$OUT_ROOT/original.png"
"$REPLAY" replay-check "$OUT_ROOT/scene.mkwr" "$OUT_ROOT/replayed.png"
cmp "$OUT_ROOT/original.png" "$OUT_ROOT/replayed.png"
echo "PASS: independent capture/replay processes produce byte-identical PNGs and pass red/blue/background pixel oracles"

"$REPLAY" capture-copies "$OUT_ROOT/copies.mkwr" "$OUT_ROOT/copies-original.png"
"$REPLAY" replay-copies-check "$OUT_ROOT/copies.mkwr" "$OUT_ROOT/copies-replayed.png"
cmp "$OUT_ROOT/copies-original.png" "$OUT_ROOT/copies-replayed.png"
echo "PASS: EFB copy, clear, optimized direct draw, GPU copy sampling and destination retirement survive replay"

"$REPLAY" capture-wide "$OUT_ROOT/wide.mkwr" "$OUT_ROOT/wide-original.png"
"$REPLAY" replay-check "$OUT_ROOT/wide.mkwr" "$OUT_ROOT/wide-replayed.png"
cmp "$OUT_ROOT/wide-original.png" "$OUT_ROOT/wide-replayed.png"
echo "PASS: variable framebuffer dimensions and padded GPU readback rows survive replay"

"$REPLAY" capture-indexed "$OUT_ROOT/indexed.mkwr" "$OUT_ROOT/indexed-original.png"
"$REPLAY" replay-check "$OUT_ROOT/indexed.mkwr" "$OUT_ROOT/indexed-replayed.png"
cmp "$OUT_ROOT/indexed-original.png" "$OUT_ROOT/indexed-replayed.png"
echo "PASS: indexed positions and UV arrays relocate and render the expected pixels"
