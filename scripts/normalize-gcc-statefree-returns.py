#!/usr/bin/env python3
"""Normalize WiiCompiled two-word state-free returns for devkitA64 GCC.

Pinned WiiCompiled emits `return { a, b };` from functions returning
MkwStateFreeResult2. Clang accepts that for ext_vector_type(2), while GCC's
vector_size equivalent requires an explicit destination type:
`return MkwStateFreeResult2{ a, b };`.

This script only rewrites bare aggregate returns while lexically inside a
function whose declared return type is MkwStateFreeResult2. It is idempotent
and intended for generated local build shards only.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

# Pinned generator syntax is typically:
#   extern "C" MKW_PPC_NO_INLINE MkwStateFreeResult2 func_xxx_statefree(...)
# but keep this tolerant of other prefixes/attributes while anchoring on the
# exact return type + function identifier pair.
FUNC_START = re.compile(r"\bMkwStateFreeResult2\s+[A-Za-z_][A-Za-z0-9_]*\s*\(")
BARE_RETURN = re.compile(r"\breturn\s*\{")


def brace_delta(line: str) -> int:
    """Count braces outside strings/comments for generated C++ lines."""
    delta = 0
    i = 0
    in_string = False
    in_char = False
    escape = False
    while i < len(line):
        ch = line[i]
        nxt = line[i + 1] if i + 1 < len(line) else ""
        if escape:
            escape = False
            i += 1
            continue
        if in_string:
            if ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
            i += 1
            continue
        if in_char:
            if ch == "\\":
                escape = True
            elif ch == "'":
                in_char = False
            i += 1
            continue
        if ch == "/" and nxt == "/":
            break
        if ch == '"':
            in_string = True
        elif ch == "'":
            in_char = True
        elif ch == "{":
            delta += 1
        elif ch == "}":
            delta -= 1
        i += 1
    return delta


def normalize_file(path: pathlib.Path) -> tuple[int, int]:
    original = path.read_text(encoding="utf-8")
    lines = original.splitlines(keepends=True)
    out: list[str] = []
    pending = False
    active = False
    depth = 0
    replacements = 0
    functions = 0

    for line in lines:
        if not active and not pending and FUNC_START.search(line):
            pending = True

        if pending or active:
            # A forward declaration/prototype is not a function body.
            if pending and ";" in line and "{" not in line:
                pending = False
            else:
                line_delta = brace_delta(line)
                if pending and "{" in line:
                    pending = False
                    active = True
                    functions += 1
                    depth = line_delta
                elif active:
                    depth += line_delta

                if active:
                    line, count = BARE_RETURN.subn("return MkwStateFreeResult2{", line)
                    replacements += count
                    if depth <= 0:
                        active = False
                        depth = 0

        out.append(line)

    rewritten = "".join(out)
    if rewritten != original:
        path.write_text(rewritten, encoding="utf-8")
    return functions, replacements


def iter_cpp(paths: list[pathlib.Path]):
    for root in paths:
        if not root.exists():
            continue
        if root.is_file() and root.suffix == ".cpp":
            yield root
        elif root.is_dir():
            yield from sorted(root.glob("*.cpp"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", type=pathlib.Path)
    args = parser.parse_args()

    files = list(iter_cpp(args.paths))
    if not files:
        print("error: no .cpp shard files found", file=sys.stderr)
        return 2

    total_functions = 0
    total_replacements = 0
    for path in files:
        functions, replacements = normalize_file(path)
        total_functions += functions
        total_replacements += replacements

    print(
        f"GCC state-free return normalization: files={len(files)} "
        f"functions={total_functions} replacements={total_replacements}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
