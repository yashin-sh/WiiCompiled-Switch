"""Apply audited strict-C++ libnx portability fixes required by Dawn/Tint.

The pinned sources rely on POSIX/newlib extensions that are hidden while the
Switch cross-build intentionally uses strict -std=c++20. Keep upstream behavior
on every other platform and add only narrow __SWITCH__ compatibility shims.

The transformations are strict and idempotent: they accept only the exact
pinned source contracts and fail instead of guessing if those sources move.
"""

import argparse
import os
import re
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

UAM_TARGET = Path("src/dawn/native/deko3d/UamCompilerDeko3d.cpp")
UAM_INCLUDE_UPSTREAM = "#include <mutex>\n#include <vector>\n"
UAM_INCLUDE_SWITCH = "#include <cstdint>\n#include <mutex>\n#include <vector>\n"
UAM_UPSTREAM = """    std::vector<uint8_t> bytes;
    compiler.OutputDkshToMemory(bytes);
    dksh->assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
"""
UAM_SWITCH = """    // Pinned libuam takes caller-owned memory and aligns absolute pointers
    // to 256 bytes while serializing DKSH. Its base must have that alignment.
    const size_t size = compiler.CalculateDkshSize();
    std::vector<uint8_t> storage(size + 255);
    const auto aligned = (reinterpret_cast<std::uintptr_t>(storage.data()) + 255) &
                         ~std::uintptr_t{255};
    auto* bytes = reinterpret_cast<uint8_t*>(aligned);
    compiler.OutputDkshToMemory(bytes);
    dksh->assign(reinterpret_cast<const char*>(bytes), size);
"""

MESSAGE_TARGET = Path("src/dawn/native/utils/WGPUHelpers.cpp")
MESSAGE_UPSTREAM = "    return std::string_view(in.data, DAWN_UNSAFE_TODO(strnlen(in.data, in.length)));\n"
MESSAGE_SWITCH = """#if defined(__SWITCH__)
    // strnlen() is hidden by strict C++20 in newlib. Stop at the first NUL
    // or the explicit bound; WGPU_STRLEN also works for terminated strings.
    size_t length = 0;
    while (length < in.length && DAWN_UNSAFE_TODO(in.data[length]) != '\\0') {
        ++length;
    }
    return std::string_view(in.data, length);
#else
    return std::string_view(in.data, DAWN_UNSAFE_TODO(strnlen(in.data, in.length)));
#endif
"""

TARGETS = (TINT_TARGET, SYSTEM_TARGET, SLEEP_TARGET, UAM_TARGET, MESSAGE_TARGET)


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


def patch_uam(root: Path, check_only: bool) -> bool:
    include_changed = patch_exact(
        root / UAM_TARGET,
        UAM_INCLUDE_UPSTREAM,
        UAM_INCLUDE_SWITCH,
        "pinned UAM serialization include",
        check_only,
    )
    output_changed = patch_exact(
        root / UAM_TARGET,
        UAM_UPSTREAM,
        UAM_SWITCH,
        "pinned UAM aligned serialization adapter",
        check_only,
    )
    return include_changed or output_changed


def patch_message(root: Path, check_only: bool) -> bool:
    return patch_exact(
        root / MESSAGE_TARGET,
        MESSAGE_UPSTREAM,
        MESSAGE_SWITCH,
        "Switch bounded message normalization",
        check_only,
    )


def base_uam_source(patch: Path) -> str:
    # This newly introduced file is one complete hunk in the pinned patch.
    # Its full contents, rather than a few matching markers, remain audited
    # when excluding it from git's reverse check after applying our adapter.
    marker = f"diff --git a/{UAM_TARGET} b/{UAM_TARGET}\n"
    source = patch.read_text(encoding="utf-8")
    if source.count(marker) != 1:
        fail("pinned Dawn patch must introduce exactly one UAM compiler file")
    section = source.split(marker, 1)[1].split("\ndiff --git ", 1)[0]
    match = re.search(r"^@@ -0,0 \+1,(\d+) @@\n", section, re.MULTILINE)
    if match is None:
        fail("pinned Dawn UAM file must be a complete new-file hunk")
    lines = section[match.end() :].splitlines()
    if len(lines) != int(match[1]) or any(not line.startswith("+") for line in lines):
        fail("pinned Dawn UAM file hunk changed")
    return "".join(line[1:] + "\n" for line in lines)


def verify_base_patch(root: Path, patch: Path) -> None:
    original = base_uam_source(patch)
    adapted = original
    for upstream, switch in (
        (UAM_INCLUDE_UPSTREAM, UAM_INCLUDE_SWITCH),
        (UAM_UPSTREAM, UAM_SWITCH),
    ):
        if adapted.count(upstream) != 1:
            fail("pinned Dawn patch UAM adapter source contract changed")
        adapted = adapted.replace(upstream, switch, 1)
    path = root / UAM_TARGET
    if not path.is_file() or path.read_text(encoding="utf-8") not in (
        original,
        adapted,
    ):
        fail("Dawn UAM compiler differs from the complete audited base/adapter file")
    result = subprocess.run(
        [
            "git",
            "-C",
            str(root),
            "apply",
            "--reverse",
            "--check",
            "--exclude=third_party/EGL-Registry/src",
            "--exclude=third_party/abseil-cpp",
            f"--exclude={UAM_TARGET}",
            str(patch),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        fail("pinned Dawn/Deko3D base patch does not reverse cleanly")


def verify(root: Path) -> None:
    for target in TARGETS:
        if not (root / target).is_file():
            fail(f"not an initialized pinned Dawn checkout: missing {target}")
    patch_tint(root, check_only=True)
    patch_system_utils(root, check_only=True)
    patch_sleep(root, check_only=True)
    patch_uam(root, check_only=True)
    patch_message(root, check_only=True)


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
            UAM_TARGET: UAM_INCLUDE_UPSTREAM + UAM_UPSTREAM,
            MESSAGE_TARGET: MESSAGE_UPSTREAM,
        }
        for target, source in fixtures.items():
            path = root / target
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(source, encoding="utf-8")
        expect_failure(lambda: verify(root))
        patchers = (
            patch_tint,
            patch_system_utils,
            patch_sleep,
            patch_uam,
            patch_message,
        )
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

        # The adapter touches a file introduced entirely by the base patch.
        # Verify both complete versions and reject changes elsewhere, retaining
        # the original reverse-apply contract for all other patch hunks.
        subprocess.run(
            ["git", "init", "--quiet", str(root)], check=True, capture_output=True
        )
        original_uam = fixtures[UAM_TARGET]
        base_patch = root / "base.patch"
        base_patch.write_text(
            f"diff --git a/{UAM_TARGET} b/{UAM_TARGET}\n"
            "new file mode 100644\n--- /dev/null\n"
            f"+++ b/{UAM_TARGET}\n"
            f"@@ -0,0 +1,{len(original_uam.splitlines())} @@\n"
            + "".join("+" + line + "\n" for line in original_uam.splitlines())
            + "diff --git a/audit.txt b/audit.txt\n"
            "--- a/audit.txt\n+++ b/audit.txt\n@@ -1 +1 @@\n-old\n+new\n",
            encoding="utf-8",
        )
        (root / "audit.txt").write_text("new\n", encoding="utf-8")
        verify_base_patch(root, base_patch)
        uam_path = root / UAM_TARGET
        uam_path.write_text(original_uam, encoding="utf-8")
        verify_base_patch(root, base_patch)
        uam_path.write_bytes(applied[UAM_TARGET] + b"// unexpected edit\n")
        expect_failure(lambda: verify_base_patch(root, base_patch))
        uam_path.write_bytes(applied[UAM_TARGET])
        (root / "audit.txt").write_text("unexpected\n", encoding="utf-8")
        expect_failure(lambda: verify_base_patch(root, base_patch))
        (root / "audit.txt").write_text("new\n", encoding="utf-8")

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
        # Exercise the adapter against the pinned pointer-based UAM ABI. The
        # stub requires absolute 256-byte alignment and writes an exact binary
        # blob, including embedded zeros, so pointer/length/padding regressions
        # fail independently of the allocation's natural alignment.
        (root / "uam_test.cpp").write_text(
            "#include <cassert>\n#include <cstddef>\n#include <cstdint>\n"
            "#include <string>\n#include <vector>\n"
            "class DekoCompiler {\npublic:\n"
            "    size_t CalculateDkshSize() const { return 512; }\n"
            "    void OutputDkshToMemory(void* mem) const {\n"
            "        assert((reinterpret_cast<std::uintptr_t>(mem) & 255) == 0);\n"
            "        auto* output = static_cast<uint8_t*>(mem);\n"
            "        for (size_t i = 0; i < 512; ++i) output[i] = i % 251;\n"
            "    }\n};\n"
            "void Emit(DekoCompiler& compiler, std::string* dksh) {\n"
            + UAM_SWITCH
            + "}\nint main() {\n"
            "    DekoCompiler compiler;\n    std::string dksh;\n"
            "    for (int repeat = 0; repeat < 16; ++repeat) {\n"
            "        Emit(compiler, &dksh);\n        assert(dksh.size() == 512);\n"
            "        for (size_t i = 0; i < dksh.size(); ++i)\n"
            "            assert(static_cast<uint8_t>(dksh[i]) == i % 251);\n"
            "    }\n}\n",
            encoding="utf-8",
        )
        subprocess.run(
            [
                compiler,
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsanitize=address,undefined",
                str(root / "uam_test.cpp"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run(
            [str(executable)],
            check=True,
            # LeakSanitizer cannot inspect threads in some traced runners.
            # Address/undefined sanitizers still cover the adapter's writes.
            env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"},
        )
        # Exercise an exact-size allocation without a trailing NUL under ASan,
        # plus explicit bounds, embedded NULs, zero length and WGPU_STRLEN.
        (root / "message_test.cpp").write_text(
            "#include <cassert>\n#include <cstddef>\n#include <limits>\n"
            "#include <memory>\n#include <string_view>\n"
            "#define DAWN_UNSAFE_TODO(value) (value)\n"
            "struct StringView { const char* data; size_t length;\n"
            "    bool IsUndefined() const {\n"
            "        return data == nullptr && length == std::numeric_limits<size_t>::max();\n"
            "    }\n};\n"
            "std::string_view NormalizeMessageString(StringView in) {\n"
            "    if (in.IsUndefined()) return {};\n"
            + MESSAGE_SWITCH
            + "}\nint main() {\n"
            "    constexpr size_t sentinel = std::numeric_limits<size_t>::max();\n"
            "    assert(NormalizeMessageString({nullptr, sentinel}).empty());\n"
            "    assert(NormalizeMessageString({nullptr, 0}).empty());\n"
            '    assert(NormalizeMessageString({"abc", 0}).empty());\n'
            '    assert(NormalizeMessageString({"abc", 2}) == "ab");\n'
            '    assert(NormalizeMessageString({"abc", sentinel}) == "abc");\n'
            '    assert(NormalizeMessageString({"", sentinel}).empty());\n'
            "    const char embedded[] = {'a', '\\0', 'b'};\n"
            '    assert(NormalizeMessageString({embedded, 3}) == "a");\n'
            "    auto bounded = std::make_unique<char[]>(1);\n"
            "    bounded[0] = 'x';\n"
            '    assert(NormalizeMessageString({bounded.get(), 1}) == "x");\n'
            "}\n",
            encoding="utf-8",
        )
        subprocess.run(
            [
                compiler,
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-D__SWITCH__",
                "-fsanitize=address,undefined",
                str(root / "message_test.cpp"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run(
            [str(executable)],
            check=True,
            env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"},
        )
    print("Dawn/Tint Switch portability self-test OK")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("dawn_root", type=Path, nargs="?")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--self-test", action="store_true")
    mode.add_argument("--verify-base-patch", type=Path)
    args = parser.parse_args()

    if args.self_test:
        if args.dawn_root is not None:
            parser.error("--self-test does not take dawn_root")
        self_test()
        return
    if args.dawn_root is None:
        parser.error("dawn_root is required unless using --self-test")

    root = args.dawn_root.resolve()
    if args.verify_base_patch is not None:
        verify_base_patch(root, args.verify_base_patch.resolve())
        print("Dawn/Deko3D complete base patch contract OK")
        return
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
        patch_uam(root, check_only=False),
        patch_message(root, check_only=False),
    ]
    verify(root)
    status = "applied" if any(changed) else "already present"
    print(f"Dawn/Tint Switch portability {status}")


if __name__ == "__main__":
    main()
