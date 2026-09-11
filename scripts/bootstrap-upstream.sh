#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
PIN=a135beb201042b20f390c6695ca6b26768820fb4
DEST="$ROOT/third_party/WiiCompiled"

git -C "$ROOT" submodule update --init --recursive third_party/WiiCompiled

ACTUAL=$(git -C "$DEST" rev-parse HEAD)
if [ "$ACTUAL" != "$PIN" ]; then
  echo "Unexpected WiiCompiled revision: $ACTUAL" >&2
  echo "Expected repository pin: $PIN" >&2
  exit 1
fi

echo "WiiCompiled ready at pinned revision $PIN"
