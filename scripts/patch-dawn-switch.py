"""Apply audited strict-C++ libnx portability fixes required by Dawn/Tint.

The pinned sources rely on POSIX/newlib extensions that are hidden while the
Switch cross-build intentionally uses strict -std=c++20. Keep upstream behavior
on every other platform and add only narrow __SWITCH__ compatibility shims.

The transformations are strict and idempotent: they accept only the exact
pinned source contracts and fail instead of guessing if those sources move.
"""

import argparse
import shutil
import subprocess
import tempfile
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

SLEEP_TARGET = Path("src/dawn/utils/SystemUtils.cpp")
SLEEP_INCLUDE_UPSTREAM = """#if DAWN_PLATFORM_IS(WINDOWS)
#include "src/utils/windows_with_undefs.h"
"""
SLEEP_INCLUDE_SWITCH = """#if defined(__SWITCH__)
#include <switch.h>
#elif DAWN_PLATFORM_IS(WINDOWS)
#include "src/utils/windows_with_undefs.h"
"""
SLEEP_UPSTREAM = """#if DAWN_PLATFORM_IS(WINDOWS)
void USleep(unsigned int usecs) {
    Sleep(static_cast<DWORD>(usecs / 1000));
}
"""
SLEEP_SWITCH = """#if defined(__SWITCH__)
void USleep(unsigned int usecs) {
    // Strict -std=c++20 hides newlib's usleep(). Horizon takes nanoseconds.
    // Widen before conversion so the full unsigned-microsecond range fits.
    svcSleepThread(static_cast<int64_t>(usecs) * 1000);
}
#elif DAWN_PLATFORM_IS(WINDOWS)
void USleep(unsigned int usecs) {
    Sleep(static_cast<DWORD>(usecs / 1000));
}
"""

TARGETS = (TINT_TARGET, SYSTEM_TARGET, SLEEP_TARGET)


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
    if text.count(switch) == 1 and upstream not in text.replace(switch, "", 1):
        return False
    if check_only:
        fail(f"{description} missing from {path}")
    if text.count(upstream) != 1 or switch in text:
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


def patch_sleep(root: Path, check_only: bool) -> bool:
    include_changed = patch_exact(
        root / SLEEP_TARGET,
        SLEEP_INCLUDE_UPSTREAM,
        SLEEP_INCLUDE_SWITCH,
        "Switch native sleep include",
        check_only,
    )
    sleep_changed = patch_exact(
        root / SLEEP_TARGET,
        SLEEP_UPSTREAM,
        SLEEP_SWITCH,
        "Switch native microsecond sleep",
        check_only,
    )
    return include_changed or sleep_changed


def verify(root: Path) -> None:
    for target in TARGETS:
        if not (root / target).is_file():
            fail(f"not an initialized pinned Dawn checkout: missing {target}")
    patch_tint(root, check_only=True)
    patch_system_utils(root, check_only=True)
    patch_sleep(root, check_only=True)


def self_test() -> None:
    compiler = shutil.which("c++")
    if compiler is None:
        fail("--self-test requires a host c++ compiler")

    def expect_failure(callback) -> None:
        try:
            callback()
        except SystemExit:
            return
        fail("self-test expected source-contract rejection")

    with tempfile.TemporaryDirectory(prefix="dawn-switch-self-test-") as directory:
        root = Path(directory)
        fixtures = {
            TINT_TARGET: TINT_UPSTREAM,
            SYSTEM_TARGET: SYSTEM_UPSTREAM,
            SLEEP_TARGET: (
                SLEEP_INCLUDE_UPSTREAM + "#endif\n\n" + SLEEP_UPSTREAM + "#endif\n"
            ),
        }
        for target, source in fixtures.items():
            path = root / target
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(source, encoding="utf-8")
        expect_failure(lambda: verify(root))
        patchers = (patch_tint, patch_system_utils, patch_sleep)
        for patcher in patchers:
            assert patcher(root, check_only=False)
        verify(root)
        applied = {target: (root / target).read_bytes() for target in TARGETS}
        for patcher in patchers:
            assert not patcher(root, check_only=False)
        assert applied == {target: (root / target).read_bytes() for target in TARGETS}

        # A repeated source anchor must fail instead of patching one copy and
        # leaving another unpatched. Check mode must also catch edited shims.
        for target, patcher in zip(TARGETS, patchers):
            path = root / target
            path.write_text(fixtures[target] * 2, encoding="utf-8")
            expect_failure(lambda patcher=patcher: patcher(root, check_only=False))
            path.write_text(fixtures[target][1:], encoding="utf-8")
            expect_failure(lambda patcher=patcher: patcher(root, check_only=False))
            path.write_bytes(applied[target] * 2)
            expect_failure(lambda patcher=patcher: patcher(root, check_only=True))
            path.write_bytes(applied[target])
        sleep_path = root / SLEEP_TARGET
        sleep_path.write_bytes(applied[SLEEP_TARGET].replace(b"* 1000", b"* 999"))
        expect_failure(lambda: verify(root))
        sleep_path.write_bytes(applied[SLEEP_TARGET])

        # Compile the patched sleep source with strict C++ and a narrow libnx
        # service stub, then verify microsecond-to-nanosecond conversion across
        # the entire unsigned input range, including the overflow boundary.
        (root / "switch.h").write_text(
            "#pragma once\n#include <cstdint>\n"
            "inline int64_t recorded_ns = -1;\n"
            "inline void svcSleepThread(int64_t ns) { recorded_ns = ns; }\n",
            encoding="utf-8",
        )
        (root / "sleep_test.cpp").write_text(
            '#include "src/dawn/utils/SystemUtils.cpp"\n'
            "#include <cassert>\n#include <limits>\n"
            "int main() {\n"
            "    static_assert(sizeof(unsigned int) == 4);\n"
            "    USleep(0); assert(recorded_ns == 0);\n"
            "    USleep(1); assert(recorded_ns == 1000);\n"
            "    USleep(std::numeric_limits<unsigned int>::max());\n"
            "    assert(recorded_ns == 4294967295000LL);\n"
            "}\n",
            encoding="utf-8",
        )
        executable = root / "sleep-test"
        subprocess.run(
            [
                compiler,
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-D__SWITCH__",
                "-I",
                str(root),
                str(root / "sleep_test.cpp"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)
    print("Dawn/Tint Switch portability self-test OK")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("dawn_root", type=Path, nargs="?")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        if args.dawn_root is not None:
            parser.error("--self-test does not take dawn_root")
        self_test()
        return
    if args.dawn_root is None:
        parser.error("dawn_root is required unless using --self-test")

    root = args.dawn_root.resolve()
    for target in TARGETS:
        if not (root / target).is_file():
            fail(f"not an initialized pinned Dawn checkout: missing {target}")

    if args.check:
        verify(root)
        print("Dawn/Tint Switch portability contract OK")
        return

    changed = [
        patch_tint(root, check_only=False),
        patch_system_utils(root, check_only=False),
        patch_sleep(root, check_only=False),
    ]
    verify(root)
    status = "applied" if any(changed) else "already present"
    print(f"Dawn/Tint Switch portability {status}")


if __name__ == "__main__":
    main()
