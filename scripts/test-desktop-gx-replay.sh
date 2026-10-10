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
"$BUILD_ROOT/frame-dump-gpu-contract" "$OUT_ROOT/frame-dump"
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

"$REPLAY" capture-direct-copies "$OUT_ROOT/direct-copies.mkwr" "$OUT_ROOT/direct-copies-original.png"
"$REPLAY" replay-direct-copies-check "$OUT_ROOT/direct-copies.mkwr" "$OUT_ROOT/direct-copies-replayed.png"
cmp "$OUT_ROOT/direct-copies-original.png" "$OUT_ROOT/direct-copies-replayed.png"
echo "PASS: RGBA8 snapshot copy uses CopySrc, survives clear and replay, and preserves sampled red pixels"

"$REPLAY" capture-wide "$OUT_ROOT/wide.mkwr" "$OUT_ROOT/wide-original.png"
"$REPLAY" replay-check "$OUT_ROOT/wide.mkwr" "$OUT_ROOT/wide-replayed.png"
cmp "$OUT_ROOT/wide-original.png" "$OUT_ROOT/wide-replayed.png"
echo "PASS: variable framebuffer dimensions and padded GPU readback rows survive replay"

"$REPLAY" capture-indexed "$OUT_ROOT/indexed.mkwr" "$OUT_ROOT/indexed-original.png"
"$REPLAY" replay-check "$OUT_ROOT/indexed.mkwr" "$OUT_ROOT/indexed-replayed.png"
cmp "$OUT_ROOT/indexed-original.png" "$OUT_ROOT/indexed-replayed.png"
echo "PASS: indexed positions and UV arrays relocate and render the expected pixels"

"$REPLAY" capture-sequence "$OUT_ROOT/sequence.mkwr" "$OUT_ROOT/sequence-original.png"
"$REPLAY" replay-sequence-check "$OUT_ROOT/sequence.mkwr" "$OUT_ROOT/sequence-replayed.png"
cmp "$OUT_ROOT/sequence-original.png" "$OUT_ROOT/sequence-replayed.png"
echo "PASS: multi-frame prefix retains state and same-address texture updates; partial third frame excluded"

"$REPLAY" capture-i4 "$OUT_ROOT/i4.mkwr" "$OUT_ROOT/i4-original.png"
"$REPLAY" replay-i4-check "$OUT_ROOT/i4.mkwr" "$OUT_ROOT/i4-replayed.png"
cmp "$OUT_ROOT/i4-original.png" "$OUT_ROOT/i4-replayed.png"
echo "PASS: partial-tile I4 textures render white/black and refresh at the same address"
"$REPLAY" capture-rgb5a3 "$OUT_ROOT/rgb5a3.mkwr" "$OUT_ROOT/rgb5a3-original.png"
"$REPLAY" replay-check "$OUT_ROOT/rgb5a3.mkwr" "$OUT_ROOT/rgb5a3-replayed.png"
cmp "$OUT_ROOT/rgb5a3-original.png" "$OUT_ROOT/rgb5a3-replayed.png"
echo "PASS: partial-tile direct RGB5A3 textures render exact red/blue pixels"

for quad_mode in quads colors; do
    "$REPLAY" "capture-lyt-$quad_mode" "$OUT_ROOT/lyt-$quad_mode.mkwr" "$OUT_ROOT/lyt-$quad_mode-original.png"
    "$REPLAY" "replay-lyt-$quad_mode-check" "$OUT_ROOT/lyt-$quad_mode.mkwr" "$OUT_ROOT/lyt-$quad_mode-replayed.png"
    cmp "$OUT_ROOT/lyt-$quad_mode-original.png" "$OUT_ROOT/lyt-$quad_mode-replayed.png"
done
echo "PASS: production LYT bridge, native borrowed display-list decoding, textured/color quads and coverage pixel oracles survive independent replay"

"$REPLAY" capture-ia8-repeat "$OUT_ROOT/ia8-repeat.mkwr" "$OUT_ROOT/ia8-repeat-original.png"
"$REPLAY" replay-ia8-repeat-check "$OUT_ROOT/ia8-repeat.mkwr" "$OUT_ROOT/ia8-repeat-replayed.png"
cmp "$OUT_ROOT/ia8-repeat-original.png" "$OUT_ROOT/ia8-repeat-replayed.png"
echo "PASS: IA8 linear repeat on both axes, outside-range UVs and same-address refresh survive independent replay"

"$REPLAY" capture-ia8-clamp "$OUT_ROOT/ia8-clamp.mkwr" "$OUT_ROOT/ia8-clamp-original.png"
"$REPLAY" replay-ia8-clamp-check "$OUT_ROOT/ia8-clamp.mkwr" "$OUT_ROOT/ia8-clamp-replayed.png"
cmp "$OUT_ROOT/ia8-clamp-original.png" "$OUT_ROOT/ia8-clamp-replayed.png"
echo "PASS: IA8 linear clamp on both axes, outside-range UVs and same-address refresh survive independent replay"
