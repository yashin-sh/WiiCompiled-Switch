#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly ROOT_DIR
TEST_DIR="$(mktemp -d)"
readonly TEST_DIR
trap 'rm -rf "$TEST_DIR"' EXIT
"${MKW_HOST_CXX:-clang++}" -std=c++20 -O2 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -DMKW_LOCAL_RENDERED_FAST_TRACK=1 -DMKW_RENDERED_FRAME_DUMP=1 \
    -I"$ROOT_DIR/include" "$ROOT_DIR/source/frame_dump_image.cpp" \
    "$ROOT_DIR/source/rendered_frame_dump.cpp" "$ROOT_DIR/tests/frame_dump_image_contract.cpp" \
    -o "$TEST_DIR/contract"
(cd "$TEST_DIR" && ./contract)
python3 - "$TEST_DIR" <<'PY'
import struct
import sys
import zlib
from pathlib import Path


def decode(path):
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    at = 8
    compressed = b""
    chunks = []
    while at < len(data):
        size = struct.unpack_from(">I", data, at)[0]
        kind = data[at + 4 : at + 8]
        body = data[at + 8 : at + 8 + size]
        checksum = struct.unpack_from(">I", data, at + 8 + size)[0]
        assert zlib.crc32(kind + body) == checksum
        chunks.append(kind)
        if kind == b"IHDR":
            width, height, *format_fields = struct.unpack(">IIBBBBB", body)
            assert format_fields == [8, 6, 0, 0, 0]
        if kind == b"IDAT":
            compressed += body
        at += size + 12
    assert at == len(data) and chunks == [b"IHDR", b"IDAT", b"IEND"]
    rows = zlib.decompress(compressed)
    assert len(rows) == (width * 4 + 1) * height
    assert all(rows[y * (width * 4 + 1)] == 0 for y in range(height))
    pixels = b"".join(rows[y * (width * 4 + 1) + 1 : (y + 1) * (width * 4 + 1)] for y in range(height))
    return width, height, pixels


root = Path(sys.argv[1])
expected = bytes([255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 128, 0, 0, 0, 255, 37, 41, 59, 255, 0, 0, 0, 0])
assert decode(root / "rgba.png") == (3, 2, expected)
assert decode(root / "bgra.png") == (3, 2, expected)
wide = bytes(channel for y in range(341) for x in range(617) for channel in (x & 255, y & 255, (x ^ y) & 255, 255))
assert decode(root / "wide.png") == (617, 341, wide)
print("PASS: independent zlib/CRC decoder verifies complete pixels, channels and padded rows")
PY
