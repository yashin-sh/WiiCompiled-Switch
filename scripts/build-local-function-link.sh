#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHARD_ROOT="$ROOT/local-product/generated/build_shards"
COMMON="$SHARD_ROOT/base_common"
SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
JOBS="${MKW_JOBS:-2}"

command -v python3 >/dev/null 2>&1 || {
  echo "error: python3 is required for the devkitA64 GCC shard compatibility pass" >&2
  exit 2
}

if [[ ! -d "$COMMON" ]]; then
  echo "error: missing $COMMON; run scripts/prepare-local-function-shards.sh first" >&2
  exit 2
fi

paths=("$COMMON")
if [[ -d "$SENSITIVE" ]]; then
  paths+=("$SENSITIVE")
fi

echo "[1/2] Normalizing pinned WiiCompiled state-free vector returns for devkitA64 GCC..."
python3 "$ROOT/scripts/normalize-gcc-statefree-returns.py" "${paths[@]}"

echo "[2/2] Building local translated-function link-only NRO..."
cd "$ROOT"
make -j"$JOBS" MKW_LOCAL_FUNCTION_SHARDS=1
