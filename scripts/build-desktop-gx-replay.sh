#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly DAWN_PIN="77029ea85250c9bdddfc2f88034afb6b5356a031"
readonly DAWN_ROOT="${MKW_REPLAY_DAWN_ROOT:-$ROOT_DIR/.deps/desktop-gx-replay-deps/dawn}"
readonly BUILD_ROOT="${MKW_REPLAY_BUILD_ROOT:-$ROOT_DIR/.deps/desktop-gx-replay}"
readonly JOBS="${MKW_JOBS:-4}"

if [[ ! -e "$DAWN_ROOT/.git" ]]; then
    mkdir -p "$(dirname "$DAWN_ROOT")"
    git clone --filter=blob:none https://github.com/danfromtico/dawn-switch.git "$DAWN_ROOT"
    git -C "$DAWN_ROOT" fetch origin "$DAWN_PIN"
    git -C "$DAWN_ROOT" checkout --detach "$DAWN_PIN"
    git -C "$DAWN_ROOT" \
        -c submodule.third_party/angle.update=none \
        -c submodule.third_party/swiftshader.update=none \
        submodule update --init --recursive --jobs "$JOBS" --depth 1
fi
if [[ "$(git -C "$DAWN_ROOT" rev-parse HEAD)" != "$DAWN_PIN" ]]; then
    echo "error: replay requires Dawn $DAWN_PIN; refusing to alter an existing checkout" >&2
    exit 1
fi
cmake -S "$ROOT_DIR/desktop-gx-replay" -B "$BUILD_ROOT" \
    -DCMAKE_BUILD_TYPE=Release -DMKW_DAWN_ROOT="$DAWN_ROOT" "$@"
cmake --build "$BUILD_ROOT" --target mkw-gx-replay replay-format-test -j "$JOBS"
ctest --test-dir "$BUILD_ROOT" --output-on-failure
