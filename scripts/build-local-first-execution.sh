#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHARD_ROOT="$ROOT/local-product/generated/build_shards"
COMMON="$SHARD_ROOT/base_common"
SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
ELF="$ROOT/WiiCompiled-Switch-local-function-exec.elf"
NRO="$ROOT/WiiCompiled-Switch-local-function-exec.nro"
MAP_ROOT="$ROOT/WiiCompiled-Switch-local-function-exec.map"
BUILD_DIR="$ROOT/build-local-function-exec"
MAP_BUILD="$BUILD_DIR/WiiCompiled-Switch-local-function-exec.map"
RETAIN_SYMBOL="func_8000609C"
JOBS="${MKW_JOBS:-2}"

command -v python3 >/dev/null 2>&1 || {
  echo "error: python3 is required for the devkitA64 GCC shard compatibility pass" >&2
  exit 2
}

NM_TOOL="${MKW_NM:-$(command -v aarch64-none-elf-nm || true)}"
if [[ -z "$NM_TOOL" && -n "${DEVKITPRO:-}" ]]; then
  NM_TOOL="$(find "$DEVKITPRO" -type f -name 'aarch64-none-elf-nm' -print -quit 2>/dev/null || true)"
fi
if [[ -z "$NM_TOOL" || ! -x "$NM_TOOL" ]]; then
  echo "error: aarch64-none-elf-nm not found; ensure devkitA64 is in PATH" >&2
  exit 2
fi

for required in \
  "$ROOT/local-product/generated/data_sections_init.cpp" \
  "$ROOT/local-product/generated/data_sections_init_blobs.S" \
  "$ROOT/local-product/generated/RuntimeConfig.h" \
  "$ROOT/local-product/generated/base_translation_output.json"; do
  if [[ ! -f "$required" ]]; then
    echo "error: missing $required; prepare local data-init/function shards first" >&2
    exit 2
  fi
done

if [[ ! -d "$COMMON" ]]; then
  echo "error: missing $COMMON; run scripts/prepare-local-function-shards.sh first" >&2
  exit 2
fi

paths=("$COMMON")
if [[ -d "$SENSITIVE" ]]; then
  paths+=("$SENSITIVE")
fi

echo "[1/3] Normalizing pinned WiiCompiled state-free vector returns for devkitA64 GCC..."
python3 "$ROOT/scripts/normalize-gcc-statefree-returns.py" "${paths[@]}"

echo "[2/3] Building guarded local first-execution NRO (direct $RETAIN_SYMBOL only)..."
rm -f "$ELF" "$NRO" "$MAP_ROOT" "$MAP_BUILD"
cd "$ROOT"
make -j"$JOBS" MKW_LOCAL_FUNCTION_EXECUTION=1

echo "[3/3] Verifying the selected translated function is defined in the final ELF..."
NM_SCAN="$(mktemp)"
trap 'rm -f "$NM_SCAN"' EXIT
"$NM_TOOL" "$ELF" >"$NM_SCAN"
if ! grep -Eq "[[:space:]]T[[:space:]]${RETAIN_SYMBOL}$" "$NM_SCAN"; then
  echo "error: final ELF does not define T $RETAIN_SYMBOL" >&2
  grep -E "[[:space:]][UT][[:space:]]${RETAIN_SYMBOL}$" "$NM_SCAN" || true
  exit 3
fi
if grep -Eq "[[:space:]]U[[:space:]]${RETAIN_SYMBOL}$" "$NM_SCAN"; then
  echo "error: final ELF still has an unresolved $RETAIN_SYMBOL" >&2
  exit 4
fi

echo "  PASS: final ELF contains T $RETAIN_SYMBOL"
echo "guarded first-execution build: PASS"
echo "output: WiiCompiled-Switch-local-function-exec.nro"
echo "This NRO executes exactly the guarded first-function probe after data init; never upload it."
