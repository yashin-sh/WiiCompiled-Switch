#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHARD_ROOT="$ROOT/local-product/generated/build_shards"
COMMON="$SHARD_ROOT/base_common"
SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
BUILD_DIR="$ROOT/build-local-function-link"
ELF="$ROOT/WiiCompiled-Switch-local-function-link.elf"
RETAIN_SYMBOL="${MKW_TRANSLATED_RETAIN_SYMBOL:-func_8000609C}"
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

if [[ ! -d "$COMMON" ]]; then
  echo "error: missing $COMMON; run scripts/prepare-local-function-shards.sh first" >&2
  exit 2
fi

paths=("$COMMON")
if [[ -d "$SENSITIVE" ]]; then
  paths+=("$SENSITIVE")
fi

echo "[1/4] Normalizing pinned WiiCompiled state-free vector returns for devkitA64 GCC..."
python3 "$ROOT/scripts/normalize-gcc-statefree-returns.py" "${paths[@]}"

echo "[2/4] Building local translated-function link-only NRO..."
cd "$ROOT"
make -j"$JOBS" MKW_LOCAL_FUNCTION_SHARDS=1 TRANSLATED_RETAIN_SYMBOL="$RETAIN_SYMBOL"

echo "[3/4] Verifying $RETAIN_SYMBOL is defined by a translated shard object..."
mapfile -t shard_objects < <(find "$BUILD_DIR" -maxdepth 1 -type f -name 'shard_*.o' -print | sort)
if [[ ${#shard_objects[@]} -eq 0 ]]; then
  echo "error: no shard_*.o objects found under $BUILD_DIR" >&2
  exit 3
fi

object_hit=""
for obj in "${shard_objects[@]}"; do
  if "$NM_TOOL" "$obj" 2>/dev/null | grep -Eq "[[:space:]]T[[:space:]]${RETAIN_SYMBOL}$"; then
    object_hit="$obj"
    break
  fi
done
if [[ -z "$object_hit" ]]; then
  echo "error: translated retain symbol $RETAIN_SYMBOL is not defined as T in any local shard object" >&2
  exit 4
fi
echo "  PASS: $(basename "$object_hit") defines T $RETAIN_SYMBOL"

echo "[4/4] Verifying $RETAIN_SYMBOL survives --gc-sections in the final ELF..."
if [[ ! -f "$ELF" ]]; then
  echo "error: expected ELF not found: $ELF" >&2
  exit 5
fi
if ! "$NM_TOOL" "$ELF" | grep -Eq "[[:space:]]T[[:space:]]${RETAIN_SYMBOL}$"; then
  echo "error: $RETAIN_SYMBOL is not retained as T in the final ELF" >&2
  "$NM_TOOL" "$ELF" | grep -E "[[:space:]][UT][[:space:]]${RETAIN_SYMBOL}$" || true
  exit 6
fi
if "$NM_TOOL" "$ELF" | grep -Eq "[[:space:]]U[[:space:]]${RETAIN_SYMBOL}$"; then
  echo "error: $RETAIN_SYMBOL remains undefined in the final ELF" >&2
  exit 7
fi

echo "  PASS: final ELF contains T $RETAIN_SYMBOL"
echo "link-only translated-function retention proof: PASS"
