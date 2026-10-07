#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly BUILD_ROOT="${MKW_REPLAY_BUILD_ROOT:-$ROOT_DIR/.deps/desktop-gx-replay}"
readonly OUT_ROOT="$BUILD_ROOT/synthetic-run"
readonly REPLAY="$BUILD_ROOT/mkw-gx-replay"

mkdir -p "$OUT_ROOT"
"$BUILD_ROOT/replay-format-test"
# Separate processes force resource pointer relocation and fresh renderer state.
"$REPLAY" capture "$OUT_ROOT/scene.mkwr" "$OUT_ROOT/original.png"
"$REPLAY" replay-check "$OUT_ROOT/scene.mkwr" "$OUT_ROOT/replayed.png"
cmp "$OUT_ROOT/original.png" "$OUT_ROOT/replayed.png"
echo "PASS: independent capture/replay processes produce byte-identical PNGs and pass red/blue/background pixel oracles"
