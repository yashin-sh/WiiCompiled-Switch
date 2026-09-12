#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="WiiCompiled-Switch-local-fast-track"
BUILD_DIR="$ROOT/build-local-fast-track"
START_SYMBOL="func_800060A4"
SENSITIVE="$ROOT/local-product/generated/build_shards/base_portable_sensitive"

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "No existing fast-track cache; run scripts/build-local-fast-track.sh once first." >&2
  exit 2
fi

if [[ -n "${MKW_JOBS:-}" ]]; then
  JOBS="$MKW_JOBS"
else
  CORES="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
  if ((CORES > 4)); then JOBS=4; else JOBS="$CORES"; fi
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
make -j"$JOBS" \
  MKW_LOCAL_FUNCTION_EXECUTION=1 \
  MKW_LOCAL_FAST_TRACK=1 \
  TARGET="$TARGET" \
  BUILD="build-local-fast-track" \
  APP_VERSION="0.0.10-local-fast-track" \
  TRANSLATED_RETAIN_SYMBOL="$START_SYMBOL" \
  SOURCES="$LOCAL_FAST_TRACK_SOURCES" \
  DEFINES='-DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_EXECUTION=1 -DMKW_LOCAL_FAST_TRACK=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1 -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1'

echo "Incremental fast-track build: PASS"
echo "output: $TARGET.nro"
