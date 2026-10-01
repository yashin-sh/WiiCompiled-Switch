#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR

readonly REPORT_TXT="$ROOT_DIR/local-product/rmcp01-dispatch-coverage.txt"
readonly REPORT_JSON="$ROOT_DIR/local-product/rmcp01-dispatch-coverage.json"

mkdir -p "$ROOT_DIR/local-product"

echo "[discovery 1/2] Scanning all local RMCP01 direct-call coverage..."
python3 "$ROOT_DIR/scripts/scan-local-rmcp01-dispatch-coverage.py"     > "$REPORT_TXT"
python3 "$ROOT_DIR/scripts/scan-local-rmcp01-dispatch-coverage.py"     --json     > "$REPORT_JSON"

echo "Static discovery reports:"
echo "  $REPORT_TXT"
echo "  $REPORT_JSON"
echo

echo "[discovery 2/2] Building rendered Discovery NRO..."
MKW_DISCOVERY_SCAN_MODE=ON     bash "$ROOT_DIR/scripts/build-local-rendered-fast-track.sh"
