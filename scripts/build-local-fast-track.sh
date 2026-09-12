#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHARD_ROOT="$ROOT/local-product/generated/build_shards"
COMMON="$SHARD_ROOT/base_common"
SENSITIVE="$SHARD_ROOT/base_portable_sensitive"
DISPATCH="$SHARD_ROOT/base_dispatch"
TARGET="WiiCompiled-Switch-local-fast-track"
ELF="$ROOT/$TARGET.elf"
NRO="$ROOT/$TARGET.nro"
BUILD_DIR="$ROOT/build-local-fast-track"
STAMP="$BUILD_DIR/.source-head"
START_SYMBOL="func_800060A4"
START_GUEST_ADDRESS="0x800060A4"
JOBS="${MKW_JOBS:-2}"
SOURCE_HEAD="$(git -C "$ROOT" rev-parse HEAD)"

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
if [[ ! -d "$DISPATCH" ]] || ! compgen -G "$DISPATCH/*.cpp" >/dev/null; then
  echo "error: missing generated base indirect dispatch under $DISPATCH" >&2
  echo "error: run scripts/prepare-local-function-shards.sh first" >&2
  exit 2
fi

paths=("$COMMON")
if [[ -d "$SENSITIVE" ]]; then
  paths+=("$SENSITIVE")
fi

echo "source HEAD: $SOURCE_HEAD"
echo "[1/4] Verifying local WiiCompiled mapping for PAL RMCP01 __start..."
MATCHES="$(grep -RIl --include='*.cpp' "$START_SYMBOL" "${paths[@]}" 2>/dev/null || true)"
if [[ -z "$MATCHES" ]]; then
  echo "error: expected $START_SYMBOL ($START_GUEST_ADDRESS) was not found in local translated shards" >&2
  echo "error: inspect local base_translation_output.json/generated shards before hardware execution" >&2
  exit 3
fi
printf '%s\n' "$MATCHES" | sed "s#^$ROOT/##" | head -n 5
printf '  PASS: local generated shards contain %s\n' "$START_SYMBOL"

echo "[2/4] Normalizing pinned WiiCompiled state-free vector returns for devkitA64 GCC..."
python3 "$ROOT/scripts/normalize-gcc-statefree-returns.py" "${paths[@]}"

echo "[3/4] Building fast-track translated startup candidate with indirect dispatch..."
rm -rf "$BUILD_DIR"
rm -f "$ELF" "$NRO" "$ROOT/$TARGET.map"
cd "$ROOT"

# The generic MKW_LOCAL_FUNCTION_EXECUTION checkpoint intentionally excludes
# registration/dispatch constructors. Fast-track now needs WiiCompiled's
# immutable base_dispatch table for real bctrl/bctr execution, so override the
# source list only for this hardware runner while continuing to exclude the
# heavier base_registration registry shards.
source_dirs=(
  source
  third_party/WiiCompiled/runtime/src/platform
  local-product-support
  local-execution-support
  local-product/generated
  local-product/generated/build_shards/base_common
  local-product/generated/build_shards/base_dispatch
)
if [[ -d "$SENSITIVE" ]]; then
  source_dirs+=(local-product/generated/build_shards/base_portable_sensitive)
fi
LOCAL_FAST_TRACK_SOURCES="${source_dirs[*]}"

make -j"$JOBS" \
  MKW_LOCAL_FUNCTION_EXECUTION=1 \
  MKW_LOCAL_FAST_TRACK=1 \
  TARGET="$TARGET" \
  BUILD="build-local-fast-track" \
  APP_VERSION="0.0.10-local-fast-track" \
  TRANSLATED_RETAIN_SYMBOL="$START_SYMBOL" \
  SOURCES="$LOCAL_FAST_TRACK_SOURCES" \
  DEFINES='-DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_FAST_TRACK=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1 -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1'

echo "[4/4] Verifying translated __start and headless fast-track marker in final ELF..."
NM_SCAN="$(mktemp)"
trap 'rm -f "$NM_SCAN"' EXIT
"$NM_TOOL" "$ELF" >"$NM_SCAN"
if ! grep -Eq "[[:space:]]T[[:space:]]${START_SYMBOL}$" "$NM_SCAN"; then
  echo "error: final ELF does not define T $START_SYMBOL" >&2
  grep -E "[[:space:]][UT][[:space:]]${START_SYMBOL}$" "$NM_SCAN" || true
  exit 4
fi
if grep -Eq "[[:space:]]U[[:space:]]${START_SYMBOL}$" "$NM_SCAN"; then
  echo "error: final ELF still has unresolved $START_SYMBOL" >&2
  exit 5
fi
if ! strings "$ELF" | grep -Fq 'PLATFORM_CONSOLE_SKIPPED_FAST_TRACK'; then
  echo "error: final ELF does not contain the headless fast-track platform marker" >&2
  echo "error: refusing to provide a hardware-test NRO from a stale/misconfigured build" >&2
  exit 6
fi

NRO_SHA256="$(sha256sum "$NRO" | awk '{print $1}')"
printf '%s\n' "$SOURCE_HEAD" >"$STAMP"

echo "  PASS: final ELF contains T $START_SYMBOL"
echo "  PASS: final ELF contains PLATFORM_CONSOLE_SKIPPED_FAST_TRACK"
echo "fast-track translated startup build: PASS"
echo "source HEAD: $SOURCE_HEAD"
echo "output: $TARGET.nro"
echo "NRO SHA-256: $NRO_SHA256"
echo "Hardware: copy THIS exact NRO; do not reuse an older local-function-exec/local-fast-track file."
echo "Hardware: launch via title override, then collect runtime-bootstrap.txt / fast-track diagnostics."
echo "This NRO contains locally generated game-derived code; never upload it."
