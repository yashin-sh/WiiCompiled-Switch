#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHARD_ROOT="$ROOT/local-product/generated/build_shards"
COMMON="$SHARD_ROOT/base_common"
SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
TARGET="WiiCompiled-Switch-local-bootstrap-prelude"
ELF="$ROOT/$TARGET.elf"
NRO="$ROOT/$TARGET.nro"
BUILD_DIR="$ROOT/build-local-bootstrap-prelude"
BOOTSTRAP_SYMBOL="func_80006210"
BOOTSTRAP_GUEST_ADDRESS="0x80006210"
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

echo "[1/4] Verifying local WiiCompiled mapping for translated __init_registers..."
MATCHES="$(grep -RIl --include='*.cpp' "$BOOTSTRAP_SYMBOL" "${paths[@]}" 2>/dev/null || true)"
if [[ -z "$MATCHES" ]]; then
  echo "error: expected $BOOTSTRAP_SYMBOL ($BOOTSTRAP_GUEST_ADDRESS) was not found in local translated shards" >&2
  echo "error: do not run the hardware probe; inspect local base_translation_output.json / generated shards first" >&2
  exit 3
fi
printf '%s\n' "$MATCHES" | sed "s#^$ROOT/##" | head -n 5
echo "  PASS: local generated shards contain $BOOTSTRAP_SYMBOL"

echo "[2/4] Normalizing pinned WiiCompiled state-free vector returns for devkitA64 GCC..."
python3 "$ROOT/scripts/normalize-gcc-statefree-returns.py" "${paths[@]}"

echo "[3/4] Building guarded translated bootstrap register prelude..."
rm -rf "$BUILD_DIR"
rm -f "$ELF" "$NRO" "$ROOT/$TARGET.map"
cd "$ROOT"
make -j"$JOBS" \
  MKW_LOCAL_FUNCTION_EXECUTION=1 \
  MKW_LOCAL_BOOTSTRAP_PRELUDE=1 \
  TARGET="$TARGET" \
  BUILD="build-local-bootstrap-prelude" \
  APP_VERSION="0.0.8-local-bootstrap-prelude" \
  TRANSLATED_RETAIN_SYMBOL="$BOOTSTRAP_SYMBOL" \
  DEFINES='-DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_BOOTSTRAP_PRELUDE=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1 -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1'

echo "[4/4] Verifying translated bootstrap helper in final ELF..."
NM_SCAN="$(mktemp)"
trap 'rm -f "$NM_SCAN"' EXIT
"$NM_TOOL" "$ELF" >"$NM_SCAN"
if ! grep -Eq "[[:space:]]T[[:space:]]${BOOTSTRAP_SYMBOL}$" "$NM_SCAN"; then
  echo "error: final ELF does not define T $BOOTSTRAP_SYMBOL" >&2
  grep -E "[[:space:]][UT][[:space:]]${BOOTSTRAP_SYMBOL}$" "$NM_SCAN" || true
  exit 4
fi
if grep -Eq "[[:space:]]U[[:space:]]${BOOTSTRAP_SYMBOL}$" "$NM_SCAN"; then
  echo "error: final ELF still has unresolved $BOOTSTRAP_SYMBOL" >&2
  exit 5
fi

echo "  PASS: final ELF contains T $BOOTSTRAP_SYMBOL"
echo "guarded translated bootstrap prelude build: PASS"
echo "output: $TARGET.nro"
echo "Hardware expectation: target=$BOOTSTRAP_GUEST_ADDRESS, translated bootstrap mode ENABLED, r3 after=0, and stop point TRANSLATED_BOOTSTRAP_PRELUDE_EXECUTED."
echo "The runtime report will expose the exact translated r1 stack value plus generated SDA2/SDA1 values."
echo "This NRO contains locally generated game-derived code; never upload it."
