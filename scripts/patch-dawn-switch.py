"""Apply audited strict-C++ libnx portability fixes required by Dawn/Tint.

The pinned sources rely on POSIX/newlib extensions that are hidden while the
Switch cross-build intentionally uses strict -std=c++20. Keep upstream behavior
on every other platform and add only narrow __SWITCH__ compatibility shims.

The transformations are strict and idempotent: they accept only the exact
pinned source contracts and fail instead of guessing if those sources move.
"""

import argparse
from pathlib import Path

TINT_TARGET = Path("src/tint/utils/text_generator/text_generator.cc")
TINT_UPSTREAM = "    auto is_ident_char = [](char c) { return std::isalnum(c) || c == '_' || !isascii(c); };\n"
TINT_SWITCH = """#ifdef __SWITCH__
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

SYSTEM_TARGET = Path("src/dawn/common/SystemUtils.cpp")
SYSTEM_UPSTREAM = """#include <array>

namespace dawn {
"""
SYSTEM_SWITCH = """#include <array>

#if defined(__SWITCH__)
// newlib provides these POSIX ABI symbols, but their declarations are hidden
// by __STRICT_ANSI__ when Dawn is compiled with strict -std=c++20.
extern "C" {
int setenv(const char* name, const char* value, int overwrite);
int unsetenv(const char* name);
}
#endif

namespace dawn {
"""


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def patch_exact(
    path: Path,
    upstream: str,
    switch: str,
    description: str,
    check_only: bool,
) -> bool:
    text = path.read_text(encoding="utf-8")
    if switch in text:
        return False
    if check_only:
        fail(f"{description} missing from {path}")
    if upstream not in text:
        fail(f"pinned source contract changed in {path}")
    path.write_text(text.replace(upstream, switch, 1), encoding="utf-8")
    return True


def patch_tint(root: Path, check_only: bool) -> bool:
    return patch_exact(
        root / TINT_TARGET,
        TINT_UPSTREAM,
        TINT_SWITCH,
        "Switch strict-C++ isascii fallback",
        check_only,
    )


def patch_system_utils(root: Path, check_only: bool) -> bool:
    return patch_exact(
        root / SYSTEM_TARGET,
        SYSTEM_UPSTREAM,
        SYSTEM_SWITCH,
        "Switch strict-C++ setenv/unsetenv declarations",
        check_only,
    )


def verify(root: Path) -> None:
    for target in (TINT_TARGET, SYSTEM_TARGET):
        if not (root / target).is_file():
            fail(f"not an initialized pinned Dawn checkout: missing {target}")
    patch_tint(root, check_only=True)
    patch_system_utils(root, check_only=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("dawn_root", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    root = args.dawn_root.resolve()
    for target in (TINT_TARGET, SYSTEM_TARGET):
        if not (root / target).is_file():
            fail(f"not an initialized pinned Dawn checkout: missing {target}")

    if args.check:
        verify(root)
        print("Dawn/Tint Switch portability contract OK")
        return

    changed = [patch_tint(root, check_only=False), patch_system_utils(root, check_only=False)]
    verify(root)
    status = "applied" if any(changed) else "already present"
    print(f"Dawn/Tint Switch portability {status}")


if __name__ == "__main__":
    main()
