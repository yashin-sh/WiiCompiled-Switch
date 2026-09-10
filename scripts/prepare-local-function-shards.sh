#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UPSTREAM="$ROOT/third_party/WiiCompiled"
PIN="a135beb201042b20f390c6695ca6b26768820fb4"
ENTRY="0x800060A4"
LOCAL_ROOT="$ROOT/local-product"
ASSETS="$LOCAL_ROOT/Assets"
MANIFEST="$LOCAL_ROOT/recomp.yml"
GENERATED="$LOCAL_ROOT/generated"
FUNCTIONS="$GENERATED/functions"
METADATA="$GENERATED/base_translation_output.json"
SHARD_ROOT="$GENERATED/build_shards"
TEMPLATE="$ROOT/local-product-support/recomp.local.yml"
CLI_PROJECT="$UPSTREAM/translator/src/Translator.Cli/Translator.Cli.csproj"
CLI_DLL="$UPSTREAM/translator/src/Translator.Cli/bin/Release/net8.0/Translator.Cli.dll"
NATIVE_SOURCES="$UPSTREAM/runtime/src"

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "error: this Switch translation checkpoint must run on Linux." >&2
  exit 2
fi

command -v git >/dev/null 2>&1 || { echo "error: git is required" >&2; exit 2; }
command -v dotnet >/dev/null 2>&1 || { echo "error: .NET 8 SDK is required" >&2; exit 2; }

cd "$ROOT"
git submodule update --init --recursive

ACTUAL_PIN="$(git -C "$UPSTREAM" rev-parse HEAD)"
if [[ "$ACTUAL_PIN" != "$PIN" ]]; then
  echo "error: WiiCompiled pin mismatch: expected $PIN, got $ACTUAL_PIN" >&2
  exit 2
fi

mkdir -p "$ASSETS" "$FUNCTIONS"
if [[ ! -f "$ASSETS/main.dol" || ! -f "$ASSETS/StaticR.rel" ]]; then
  cat >&2 <<EOF
error: user-owned PAL RMCP01 inputs are missing.
Place them only in the gitignored local directory:
  $ASSETS/main.dol
  $ASSETS/StaticR.rel
EOF
  exit 2
fi

cp "$TEMPLATE" "$MANIFEST"

echo "[1/4] Building pinned WiiCompiled translator..."
dotnet build "$CLI_PROJECT" -c Release

echo "[2/4] Validating local manifest/input revision..."
dotnet "$CLI_DLL" info --project "$MANIFEST"

echo "[3/4] Translating Mario Kart Wii recursively from $ENTRY..."
# The pinned CLI does NOT infer an output-metadata path from output.root.
# Keep every output explicit so this local workspace cannot accidentally fall
# back to <repo>/generated/, which is the desktop translator default.
dotnet "$CLI_DLL" translate-recursive "$ENTRY" \
  --project "$MANIFEST" \
  --outdir "$FUNCTIONS" \
  --output-metadata "$METADATA" \
  --prune-stale

if [[ ! -f "$METADATA" ]]; then
  echo "error: translator did not produce expected metadata: $METADATA" >&2
  exit 2
fi

echo "[4/4] Emitting stable translated build shards..."
# emit-build-shards also has desktop-workspace defaults. Pass all four local
# roots explicitly so the generated graph stays under ignored local-product/.
dotnet "$CLI_DLL" emit-build-shards \
  --project "$MANIFEST" \
  --base-metadata "$METADATA" \
  --base-functions-dir "$FUNCTIONS" \
  --native-source-dir "$NATIVE_SOURCES" \
  --out "$SHARD_ROOT"

SHARDS_CMAKE="$SHARD_ROOT/shards.cmake"
for required in "$METADATA" "$SHARDS_CMAKE"; do
  if [[ ! -f "$required" ]]; then
    echo "error: translator did not produce expected file: $required" >&2
    exit 2
  fi
done

for required_dir in base_common base_registration base_dispatch; do
  if [[ ! -d "$SHARD_ROOT/$required_dir" ]]; then
    echo "error: translator did not produce expected shard directory: $SHARD_ROOT/$required_dir" >&2
    exit 2
  fi
done

COMMON_COUNT="$(find "$SHARD_ROOT/base_common" -maxdepth 1 -type f -name '*.cpp' | wc -l)"
SENSITIVE_COUNT=0
if [[ -d "$SHARD_ROOT/base_portable_sensitive" ]]; then
  SENSITIVE_COUNT="$(find "$SHARD_ROOT/base_portable_sensitive" -maxdepth 1 -type f -name '*.cpp' | wc -l)"
fi
REGISTRATION_COUNT="$(find "$SHARD_ROOT/base_registration" -maxdepth 1 -type f -name '*.cpp' | wc -l)"
DISPATCH_COUNT="$(find "$SHARD_ROOT/base_dispatch" -maxdepth 1 -type f -name '*.cpp' | wc -l)"

BASE_FUNCTION_COUNT="$(sed -nE 's/^set\(MKW_BASE_FUNCTION_COUNT ([0-9]+)\)$/\1/p' "$SHARDS_CMAKE" | head -n1)"
if [[ -z "$BASE_FUNCTION_COUNT" ]]; then
  echo "error: could not read MKW_BASE_FUNCTION_COUNT from $SHARDS_CMAKE" >&2
  exit 2
fi
if [[ "$BASE_FUNCTION_COUNT" -le 0 || "$COMMON_COUNT" -le 0 ]]; then
  echo "error: translated base graph is unexpectedly empty" >&2
  exit 2
fi

cat <<EOF
Local translated function-shard generation complete.
  base functions             : $BASE_FUNCTION_COUNT
  base_common shards         : $COMMON_COUNT
  base_portable_sensitive    : $SENSITIVE_COUNT
  base_registration sources  : $REGISTRATION_COUNT
  base_dispatch sources      : $DISPATCH_COUNT
  build graph                : $SHARDS_CMAKE

Everything above remains under gitignored local-product/.
Do NOT upload generated C++, JSON, shards, DOL/REL inputs, or a game-containing NRO.

The next Switch link-only mode consumes only base_common and
base_portable_sensitive function shards. Registration/dispatch shards remain
excluded because their registrar objects execute static constructors before
main. No translated guest function is called by the runtime bootstrap.
EOF
