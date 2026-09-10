#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UPSTREAM="$ROOT/third_party/WiiCompiled"
PIN="a135beb201042b20f390c6695ca6b26768820fb4"
LOCAL_ROOT="$ROOT/local-product"
ASSETS="$LOCAL_ROOT/Assets"
MANIFEST="$LOCAL_ROOT/recomp.yml"
GENERATED="$LOCAL_ROOT/generated"
TEMPLATE="$ROOT/local-product-support/recomp.local.yml"
CLI_PROJECT="$UPSTREAM/translator/src/Translator.Cli/Translator.Cli.csproj"
CLI_DLL="$UPSTREAM/translator/src/Translator.Cli/bin/Release/net8.0/Translator.Cli.dll"

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "error: this Switch data-init checkpoint must be generated on Linux." >&2
  echo "WiiCompiled emits platform-specific assembly section syntax; devkitA64 expects ELF syntax." >&2
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

mkdir -p "$ASSETS"
if [[ ! -f "$ASSETS/main.dol" || ! -f "$ASSETS/StaticR.rel" ]]; then
  cat >&2 <<EOF
error: user-owned PAL RMCP01 inputs are missing.
Place exactly these locally (they are gitignored and must never be committed):
  $ASSETS/main.dol
  $ASSETS/StaticR.rel
EOF
  exit 2
fi

cp "$TEMPLATE" "$MANIFEST"

echo "[1/3] Building pinned WiiCompiled translator..."
dotnet build "$CLI_PROJECT" -c Release

echo "[2/3] Validating local manifest/input revision..."
dotnet "$CLI_DLL" info --project "$MANIFEST"

echo "[3/3] Generating embedded data-section initializer..."
dotnet "$CLI_DLL" generate-data-init --project "$MANIFEST"

CPP="$GENERATED/data_sections_init.cpp"
ASM="$GENERATED/data_sections_init_blobs.S"
CONFIG="$GENERATED/RuntimeConfig.h"

for required in "$CPP" "$ASM" "$CONFIG"; do
  if [[ ! -f "$required" ]]; then
    echo "error: translator did not produce expected file: $required" >&2
    exit 2
  fi
done

if ! grep -Fq '.section .rodata,"a",@progbits' "$ASM"; then
  echo "error: generated blob assembly is not in the expected Linux/ELF form" >&2
  exit 2
fi

cat <<EOF
Local data-init generation complete.
Generated files remain under the gitignored directory:
  $GENERATED

Build the game-containing local NRO only on your machine:
  make -j2 MKW_LOCAL_PRODUCT=1

Expected output:
  WiiCompiled-Switch-local-product.nro

Do NOT upload that NRO, the generated directory, or the DOL/REL inputs to GitHub/public CI.
EOF
