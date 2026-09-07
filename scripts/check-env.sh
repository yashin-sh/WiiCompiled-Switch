#!/usr/bin/env sh
set -eu

fail() { echo "ERROR: $*" >&2; exit 1; }

[ -n "${DEVKITPRO:-}" ] || fail "DEVKITPRO is not set"
[ -d "$DEVKITPRO/libnx" ] || fail "libnx not found under $DEVKITPRO/libnx"
command -v aarch64-none-elf-g++ >/dev/null 2>&1 || fail "devkitA64 compiler not found in PATH"

echo "DEVKITPRO=$DEVKITPRO"
echo "libnx: OK"
echo "devkitA64 compiler: OK"
echo "Environment looks ready."
