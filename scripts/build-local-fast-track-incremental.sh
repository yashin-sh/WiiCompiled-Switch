#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="WiiCompiled-Switch-local-fast-track"
BUILD_DIR="$ROOT/build-local-fast-track"
ELF="$ROOT/$TARGET.elf"
NRO="$ROOT/$TARGET.nro"
STAMP="$BUILD_DIR/.source-head"
START_SYMBOL="func_800060A4"
SENSITIVE="$ROOT/local-product/generated/build_shards/base_portable_sensitive"

if [[ -n "${MKW_JOBS:-}" ]]; then
  JOBS="$MKW_JOBS"
else
  CORES="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
  if ((CORES > 4)); then JOBS=4; else JOBS="$CORES"; fi
fi

CURRENT_HEAD="$(git -C "$ROOT" rev-parse HEAD)"
CACHED_HEAD=""
if [[ -f "$STAMP" ]]; then
  CACHED_HEAD="$(<"$STAMP")"
fi

if [[ ! -d "$BUILD_DIR" || "$CACHED_HEAD" != "$CURRENT_HEAD" ]]; then
  echo "Fast-track cache does not match current source revision."
  echo "  cached HEAD : ${CACHED_HEAD:-<none>}"
  echo "  current HEAD: $CURRENT_HEAD"
  echo "Running a clean fast-track build before hardware testing."
  MKW_JOBS="$JOBS" bash "$ROOT/scripts/build-local-fast-track.sh"
  exit $?
fi

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

cd "$ROOT"
echo "Incremental fast-track build with $JOBS jobs"
echo "source HEAD: $CURRENT_HEAD"
make -j"$JOBS" \
  MKW_LOCAL_FUNCTION_EXECUTION=1 \
  MKW_LOCAL_FAST_TRACK=1 \
  TARGET="$TARGET" \
  BUILD="build-local-fast-track" \
  APP_VERSION="0.0.10-local-fast-track" \
  TRANSLATED_RETAIN_SYMBOL="$START_SYMBOL" \
  SOURCES="$LOCAL_FAST_TRACK_SOURCES" \
  DEFINES='-DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_FAST_TRACK=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1 -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1'

if [[ ! -f "$ELF" || ! -f "$NRO" ]]; then
  echo "error: expected fast-track ELF/NRO was not produced" >&2
  exit 6
fi

if ! grep -aFq 'PLATFORM_CONSOLE_SKIPPED_FAST_TRACK' "$ELF"; then
  echo "error: final ELF does not contain the headless fast-track platform marker" >&2
  echo "error: refusing to provide a hardware-test NRO from a stale/misconfigured build" >&2
  exit 7
fi

NRO_SHA256="$(sha256sum "$NRO" | awk '{print $1}')"
printf '%s\n' "$CURRENT_HEAD" >"$STAMP"

echo "Incremental fast-track build: PASS"
echo "source HEAD: $CURRENT_HEAD"
echo "output: $TARGET.nro"
echo "NRO SHA-256: $NRO_SHA256"
echo "Hardware: copy THIS exact NRO; the verified ELF contains PLATFORM_CONSOLE_SKIPPED_FAST_TRACK."
