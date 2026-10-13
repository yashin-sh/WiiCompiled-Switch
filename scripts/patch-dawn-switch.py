"""Apply the audited libnx portability fix required by Dawn/Tint.

Dawn's pinned Tint source calls the non-ISO isascii() extension while the
Switch cross-build intentionally uses strict -std=c++20. devkitA64/newlib hides
that declaration under __STRICT_ANSI__, so keep upstream behavior everywhere
else and use an equivalent byte-range test only for __SWITCH__.

The transformation is strict and idempotent: it only accepts the exact pinned
source contract and fails instead of guessing if that source moves.
"""

import argparse
from pathlib import Path

TARGET = Path("src/tint/utils/text_generator/text_generator.cc")
UPSTREAM = (
    "    auto is_ident_char = [](char c) { return std::isalnum(c) || c == '_' || !isascii(c); };\n"
)
SWITCH = """#ifdef __SWITCH__
    // isascii() is a non-ISO newlib extension hidden by strict -std=c++20.
    // UTF-8 continuation/leading bytes are identifier bytes here, matching
    // the upstream !isascii(c) behavior without enabling GNU extensions.
    auto is_ident_char = [](char c) {
        const auto uc = static_cast<unsigned char>(c);
        return std::isalnum(uc) || c == '_' || uc > 0x7f;
    };
#else
    auto is_ident_char = [](char c) { return std::isalnum(c) || c == '_' || !isascii(c); };
#endif
"""


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def patch(path: Path, check_only: bool) -> bool:
    text = path.read_text(encoding="utf-8")
    if SWITCH in text:
        return False
    if check_only:
        fail(f"Switch strict-C++ isascii fallback missing from {path}")
    if UPSTREAM not in text:
        fail(f"Tint identifier predicate contract changed in {path}")
    path.write_text(text.replace(UPSTREAM, SWITCH, 1), encoding="utf-8")
    return True


def verify(root: Path) -> None:
    path = root / TARGET
    if not path.is_file():
        fail(f"not an initialized pinned Dawn checkout: {root}")
    patch(path, check_only=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("dawn_root", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    root = args.dawn_root.resolve()
    path = root / TARGET
    if not path.is_file():
        fail(f"not an initialized pinned Dawn checkout: {root}")

    if args.check:
        verify(root)
        print(f"Dawn Tint Switch portability contract OK: {path}")
        return

    changed = patch(path, check_only=False)
    verify(root)
    status = "applied" if changed else "already present"
    print(f"Dawn Tint Switch portability {status}: {path}")


if __name__ == "__main__":
    main()
