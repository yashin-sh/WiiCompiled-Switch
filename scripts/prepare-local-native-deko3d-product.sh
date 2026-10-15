#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
readonly REF_DIR="$ROOT_DIR/third_party/DriftDroid"
readonly REF_PIN="8b33d31016a8b8735fe35b4fb633c491d33774c6"
readonly DEPS_DIR="${MKW_DEKO3D_DEPS_DIR:-$ROOT_DIR/.deps/native-deko3d}"
readonly WORK_DIR="${MKW_DEKO3D_WORK_DIR:-$DEPS_DIR/driftdroid}"
readonly ASSET_DIR="${MKW_LOCAL_ASSET_DIR:-$ROOT_DIR/local-product/Assets}"
readonly JOBS="${MKW_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: missing required command: $1" >&2
        exit 1
    fi
}

need git
need tar
need dotnet
need sha256sum
need python3

# Resolve aliases before destructive cleanup, and preserve the checkout, inputs
# and other products even when callers override the workspace location.
python3 - "$WORK_DIR" "$ROOT_DIR" "$ASSET_DIR" "$REF_DIR" "$DEPS_DIR" \
    "${MKW_DEKO3D_OUTPUT_DIR:-$ROOT_DIR/local-product/native-deko3d}" <<'PY'
from pathlib import Path
import subprocess
import sys

work, root, assets, reference, deps, output = map(lambda p: Path(p).resolve(), sys.argv[1:])
for protected in (root, assets, reference, deps, output):
    if work == protected or work in protected.parents:
        raise SystemExit(f"error: workspace cleanup would remove protected path: {protected}")
for protected in (assets, reference):
    if protected in work.parents:
        raise SystemExit(f"error: workspace must be separate from protected path: {protected}")
if root in work.parents:
    ignored = subprocess.run(["git", "-C", str(root), "check-ignore", "-q", str(work)])
    if ignored.returncode != 0:
        raise SystemExit("error: generated workspace inside the repository must be Git-ignored")
PY

"$ROOT_DIR/scripts/prepare-native-deko3d-reference.sh"

for name in main.dol StaticR.rel; do
    if [[ ! -f "$ASSET_DIR/$name" ]]; then
        echo "error: missing user-owned game input: $ASSET_DIR/$name" >&2
        exit 2
    fi
done

# Work from a clean archive of the exact public pin. Never translate inside the
# git submodule: generated sources and user-owned bytes must remain outside Git.
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
git -C "$REF_DIR" archive --format=tar "$REF_PIN" | tar -xf - -C "$WORK_DIR"
mkdir -p "$WORK_DIR/Assets"
cp "$ASSET_DIR/main.dol" "$WORK_DIR/Assets/main.dol"
cp "$ASSET_DIR/StaticR.rel" "$WORK_DIR/Assets/StaticR.rel"

python3 "$ROOT_DIR/scripts/patch-native-deko3d-product.py" "$WORK_DIR" --reference "$REF_DIR"

project="$WORK_DIR/projects/mkwii/recomp.yml"
translator_project="$WORK_DIR/translator/src/Translator.Cli/Translator.Cli.csproj"
translator="$WORK_DIR/translator/src/Translator.Cli/bin/Release/net8.0/Translator.Cli.dll"
generated="$WORK_DIR/generated"
functions="$generated/functions"
metadata="$generated/base_translation_output.json"
base_manifest_dir="$WORK_DIR/build/base"
shards="$generated/build_shards"

mkdir -p "$generated" "$functions" "$base_manifest_dir" "$shards"

echo "==> Building pinned static translator"
dotnet build "$translator_project" -c Release --nologo
if [[ ! -f "$translator" ]]; then
    echo "error: translator output not found: $translator" >&2
    exit 3
fi

pushd "$WORK_DIR" >/dev/null

echo "==> Translating PAL RMCP01 PowerPC code to the native build graph"
dotnet "$translator" translate-recursive 0x800060A4 \
    --project "$project" \
    --outdir "$functions" \
    --output-metadata "$metadata" \
    --production-source-bundle "$generated/base_translation_sources.bin" \
    --no-function-files \
    --prune-stale \
    --threads "$JOBS"

echo "==> Emitting base manifest"
dotnet "$translator" emit-base-manifest \
    --project "$project" \
    --out "$base_manifest_dir" \
    --functions-dir "$functions" \
    --translation-output-metadata "$metadata" \
    --region P

echo "==> Generating guest data initialization"
dotnet "$translator" generate-data-init --project "$project"

echo "==> Emitting content-addressed native build shards"
dotnet "$translator" emit-build-shards \
    --project "$project" \
    --base-metadata "$metadata" \
    --base-functions-dir "$functions" \
    --native-source-dir "$WORK_DIR/runtime/src" \
    --out "$shards"

popd >/dev/null

{
    echo "reference_commit=$REF_PIN"
    echo "main_dol_sha256=$(sha256sum "$ASSET_DIR/main.dol" | awk '{print $1}')"
    echo "staticr_rel_sha256=$(sha256sum "$ASSET_DIR/StaticR.rel" | awk '{print $1}')"
} > "$WORK_DIR/.wcs-native-deko3d-provenance"

cat <<EOF
Native Deko3D translation workspace prepared.
  workspace : $WORK_DIR
  reference : $REF_PIN
  project   : $project

No game data or generated translation was written into the Git worktree.
Next: scripts/build-local-native-deko3d.sh
EOF
