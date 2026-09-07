#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$ROOT/third_party/Wiicompiled"

if [ -e "$DEST/.git" ]; then
  echo "WiiCompiled already present: $DEST"
  exit 0
fi

git clone https://github.com/patchzyy/Wiicompiled.git "$DEST"
echo "Cloned upstream WiiCompiled. Pin a commit before beginning integration."
