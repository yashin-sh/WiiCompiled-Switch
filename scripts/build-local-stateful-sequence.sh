#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHARD_ROOT="$ROOT/local-product/generated/build_shards"
COMMON="$SHARD_ROOT/base_common"
SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
TARGET="WiiCompiled-Switch-local-stateful-sequence"
ELF="$ROOT/$TARGET.elf"
NRO="$ROOT/$TARGET.nro"
MAP_ROOT="$ROOT/$TARGET.map"
BUILD_DIR="$ROOT/build-local-stateful-sequence"
MAP_BUILD="$BUILD_DIR/$TARGET.map"
SETTER_SYMBOL="func_80006090"
GETTER_SYMBOL="func_8000609C"
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

echo "[2/3] Building guarded two-function stateful sequence..."
rm -rf "$BUILD_DIR"
rm -f "$ELF" "$NRO" "$MAP_ROOT" "$MAP_BUILD"
cd "$ROOT"
make -j"$JOBS" \
  MKW_LOCAL_FUNCTION_EXECUTION=1 \
  MKW_LOCAL_FUNCTION_SEQUENCE=1 \
  TARGET="$TARGET" \
  BUILD="build-local-stateful-sequence" \
  APP_VERSION="0.0.7-local-sequence" \
  TRANSLATED_RETAIN_SYMBOL="$SETTER_SYMBOL" \
  DEFINES='-DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_FUNCTION_SEQUENCE=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1 -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1'

echo "[3/3] Verifying both real translated functions are defined in the final ELF..."
NM_SCAN="$(mktemp)"
trap 'rm -f "$NM_SCAN"' EXIT
"$NM_TOOL" "$ELF" >"$NM_SCAN"

for symbol in "$SETTER_SYMBOL" "$GETTER_SYMBOL"; do
  if ! grep -Eq "[[:space:]]T[[:space:]]${symbol}$" "$NM_SCAN"; then
    echo "error: final ELF does not define T $symbol" >&2
    grep -E "[[:space:]][UT][[:space:]]${symbol}$" "$NM_SCAN" || true
    exit 3
  fi
  if grep -Eq "[[:space:]]U[[:space:]]${symbol}$" "$NM_SCAN"; then
    echo "error: final ELF still has unresolved $symbol" >&2
    exit 4
  fi
  echo "  PASS: final ELF contains T $symbol"
done

echo "guarded stateful translated sequence build: PASS"
echo "output: $TARGET.nro"
echo "Hardware expectation: start target 0x80006090 and final translated exec r3 after=0x00000001."
echo "This NRO contains locally generated game-derived code; never upload it."
