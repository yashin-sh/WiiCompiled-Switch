#!/usr/bin/env python3
"""Apply audited link/SQLite fixes only to the private pinned product workspace."""

import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

REFERENCE_PIN = "8b33d31016a8b8735fe35b4fb633c491d33774c6"
SQLITE_ORIGINAL = "9512509b1bccb7461f79bea8aad6280ae4699e925fa4804381b71f59e7efb0c5"
SQLITE_PATCHED = "3a3e6c80851e2336bd083b32234b0ca06027c72b45bcb3f83f58790bdcb89709"
SQLITE_PATCH = "3497970385f788566aec180f913b2f43d931fc7a3b55d0fcf0e025dc75c2250a"
SYMBOL_DECLARATIONS = """// generated/guest_symbol_table.cpp (or the stub when it is absent): the
// project's function-map names, sorted by address, for crash symbolization.
extern "C" {
extern const uint32_t kGuestMapSymbolCount;
extern const uint32_t kGuestMapSymbolAddresses[];
extern const char* const kGuestMapSymbolNames[];
}
"""
SQLITE_OLD_COMMAND = "PATCH_COMMAND patch -p1 -N -r - < ${CMAKE_CURRENT_LIST_DIR}/../cmake/patches/sqlite3-nintendo-switch-platform.patch || ${CMAKE_COMMAND} -E true"
SQLITE_NEW_COMMAND = '''PATCH_COMMAND "${MKW_NATIVE_DEKO3D_PYTHON}" "${MKW_NATIVE_DEKO3D_PRODUCT_PATCHER}"
        --sqlite sqlite3.c --sqlite-patch
        "${CMAKE_CURRENT_LIST_DIR}/../cmake/patches/sqlite3-nintendo-switch-platform.patch"'''
EXCEPTION_STACK = """// libnx defaults its user-exception stack to only 0x400 bytes. newlib's
// unbuffered vfprintf uses a BUFSIZ-sized temporary FILE buffer on the stack;
// the old handler overflowed before its first diagnostic reached the SD card.
extern "C" {
alignas(16) u8 __nx_exception_stack[64 * 1024] = {};
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
}

"""
FRAME_READ_GUARD = """        // A corrupt frame chain must not fault again inside the crash logger.
        MemoryInfo frameMemory{};
        u32 framePageInfo = 0;
        if (R_FAILED(svcQueryMemory(&frameMemory, &framePageInfo, fp)) ||
            (frameMemory.perm & Perm_R) == 0 || fp < frameMemory.addr ||
            frameMemory.size < 2 * sizeof(u64) ||
            fp - frameMemory.addr > frameMemory.size - 2 * sizeof(u64)) {
            break;
        }
"""


def digest(data):
    return hashlib.sha256(data).hexdigest()


def replace_once(source, before, after):
    if source.count(before) != 1:
        raise ValueError(f"expected one pinned context: {before[:100]!r}")
    return source.replace(before, after, 1)


def product_changes(reference):
    changes = {}
    name = "runtime/src/system_bridge.cpp"
    original = (reference / name).read_text()
    patched = replace_once(original, SYMBOL_DECLARATIONS + "\n", "")
    patched = replace_once(
        patched, "namespace {\n", SYMBOL_DECLARATIONS + "\nnamespace {\n"
    )
    changes[name] = (original, patched)
    name = "aurora-main/lib/webgpu/gpu.cpp"
    original = (reference / name).read_text()
    old_guard = "#if defined(__SWITCH__) || (defined(__ANDROID__) && defined(DAWN_ENABLE_BACKEND_OPENGLES))\n"
    new_guard = "#if (defined(__SWITCH__) || defined(__ANDROID__)) && defined(DAWN_ENABLE_BACKEND_OPENGLES)\n"
    patched = original
    for suffix in (
        "#include <dawn/native/OpenGLBackend.h>\n",
        "    // Dawn's GL backend only offers Compatibility-level adapters, and wants an explicit entry\n",
    ):
        patched = replace_once(patched, old_guard + suffix, new_guard + suffix)
    changes[name] = (original, patched)
    name = "aurora-main/cmake/AuroraDawnProvider.cmake"
    original = (reference / name).read_text()
    switch, rest = original.split("  return()\nendif ()", 1)
    switch = replace_once(
        switch,
        'set(DAWN_ENABLE_VULKAN OFF CACHE INTERNAL "")',
        'set(DAWN_ENABLE_VULKAN OFF CACHE INTERNAL "" FORCE)',
    )
    switch = replace_once(
        switch,
        'set(DAWN_ENABLE_OPENGLES ON CACHE INTERNAL "")',
        'set(DAWN_ENABLE_OPENGLES OFF CACHE INTERNAL "" FORCE)\n  set(DAWN_ENABLE_DEKO3D ON CACHE INTERNAL "" FORCE)',
    )
    changes[name] = (original, switch + "  return()\nendif ()" + rest)
    name = "aurora-main/cmake/aurora_core.cmake"
    original = (reference / name).read_text()
    changes[name] = (
        original,
        replace_once(
            original,
            "    if (DAWN_ENABLE_VULKAN)\n",
            "    if (DAWN_ENABLE_DEKO3D)\n        target_compile_definitions(aurora_core PRIVATE DAWN_ENABLE_BACKEND_DEKO3D)\n    endif ()\n    if (DAWN_ENABLE_VULKAN)\n",
        ),
    )
    name = "aurora-main/lib/aurora.cpp"
    original = (reference / name).read_text()
    changes[name] = (
        original,
        replace_once(
            original,
            "#if defined(__SWITCH__) && defined(DAWN_ENABLE_BACKEND_OPENGLES)\n    // Switch's only backend: Dawn's GLES backend on switch-mesa (deko3d comes later for 60fps).\n    BACKEND_OPENGLES,\n#endif\n",
            "#if defined(__SWITCH__) && defined(DAWN_ENABLE_BACKEND_DEKO3D)\n    // This Switch archive exposes the native Deko3D backend.\n    BACKEND_DEKO3D,\n#endif\n",
        ),
    )
    name = "aurora-main/extern/CMakeLists.txt"
    original = (reference / name).read_text()
    changes[name] = (
        original,
        replace_once(original, SQLITE_OLD_COMMAND, SQLITE_NEW_COMMAND),
    )
    name = "runtime/src/main.cpp"
    original = (reference / name).read_text()
    patched = replace_once(
        original,
        "// hbloader runs every homebrew .nro inside its own process, so an unhandled CPU fault produces\n",
        EXCEPTION_STACK
        + "// hbloader runs every homebrew .nro inside its own process, so an unhandled CPU fault produces\n",
    )
    patched = replace_once(
        patched,
        "        const auto* entry = reinterpret_cast<const u64*>(fp);\n",
        FRAME_READ_GUARD
        + "        const auto* entry = reinterpret_cast<const u64*>(fp);\n",
    )
    changes[name] = (original, patched)
    return changes


def apply_product(workspace, reference, check=False):
    workspace, reference = workspace.resolve(), reference.resolve()
    if (
        workspace == reference
        or workspace in reference.parents
        or reference in workspace.parents
    ):
        raise ValueError("product workspace must be separate from the pinned reference")
    pin = subprocess.check_output(
        ["git", "-C", str(reference), "rev-parse", "HEAD"], text=True
    ).strip()
    if pin != REFERENCE_PIN:
        raise ValueError("product reference pin mismatch")
    if subprocess.check_output(
        ["git", "-C", str(reference), "status", "--porcelain"], text=True
    ).strip():
        raise ValueError("product reference must be clean")
    changes = product_changes(reference)
    # Validate every file before writing any; reject edits outside the authorized delta.
    for name, (original, patched) in changes.items():
        actual = (workspace / name).read_text()
        if actual not in (original, patched) or (check and actual != patched):
            raise ValueError(f"unexpected product source delta: {name}")
    if not check:
        for name, (original, patched) in changes.items():
            path = workspace / name
            if path.read_text() != patched:
                path.write_text(patched)
        receipt = {
            "reference_commit": pin,
            "patcher_sha256": digest(Path(__file__).read_bytes()),
            "files": {
                name: {
                    "original_sha256": digest(original.encode()),
                    "patched_sha256": digest(patched.encode()),
                }
                for name, (original, patched) in changes.items()
            },
        }
        (workspace / ".wcs-native-deko3d-product-patches.json").write_text(
            json.dumps(receipt, indent=2) + "\n"
        )


def sqlite_hunks(patch):
    old, new, hunks, active = [], [], [], False
    for line in patch.splitlines(keepends=True):
        if line.startswith(("@@ ", "--- ", "+++ ")):
            if active:
                hunks.append(("".join(old), "".join(new)))
            old, new = [], []
            active = line.startswith("@@ ")
        elif active:
            if not line or line[0] not in " +-":
                raise ValueError("unexpected pinned SQLite patch line")
            if line[0] in " -":
                old.append(line[1:])
            if line[0] in " +":
                new.append(line[1:])
    if active:
        hunks.append(("".join(old), "".join(new)))
    if len(hunks) != 3:
        raise ValueError("expected the three pinned SQLite portability hunks")
    return hunks


def apply_sqlite(source, patch, check=False):
    patch_bytes, source_bytes = patch.read_bytes(), source.read_bytes()
    if digest(patch_bytes) != SQLITE_PATCH:
        raise ValueError("SQLite reference patch hash mismatch")
    actual = digest(source_bytes)
    if actual == SQLITE_PATCHED:
        return
    if actual != SQLITE_ORIGINAL or check:
        raise ValueError(
            "SQLite source is not the attributed original/patched amalgamation"
        )
    # The reference has malformed unified-diff counts. Match every complete literal
    # hunk exactly, without offsets/fuzz, and pin both complete source hashes.
    text = source_bytes.decode()
    for original, patched in sqlite_hunks(patch_bytes.decode()):
        text = replace_once(text, original, patched)
    encoded = text.encode()
    if digest(encoded) != SQLITE_PATCHED:
        raise ValueError("complete patched SQLite hash mismatch")
    source.write_bytes(encoded)


def self_test(reference):
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory) / "work"
        for name in product_changes(reference):
            target = work / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((reference / name).read_bytes())
        apply_product(work, reference)
        hashes = {
            name: digest((work / name).read_bytes())
            for name in product_changes(reference)
        }
        apply_product(work, reference)
        apply_product(work, reference, check=True)
        assert all(
            digest((work / name).read_bytes()) == value
            for name, value in hashes.items()
        )
        gpu = work / "aurora-main/lib/webgpu/gpu.cpp"
        gpu.write_text(gpu.read_text() + "// unauthorized change\n")
        before = gpu.read_bytes()
        try:
            apply_product(work, reference)
        except ValueError:
            pass
        else:
            raise AssertionError("unattributed product edit accepted")
        assert gpu.read_bytes() == before
        try:
            apply_product(reference, reference)
        except ValueError:
            pass
        else:
            raise AssertionError("reference mutation accepted")
        # Compile the complete production exception handler, and link a second
        # TU with libnx's weak stack defaults to verify the actual override ABI.
        from dawn_deko3d_runtime import method

        runtime_main = product_changes(reference)["runtime/src/main.cpp"][1]
        (work / "exception-production.inc").write_text(
            EXCEPTION_STACK
            + method(runtime_main, 'extern "C" void __libnx_exception_handler(')
        )
        defaults = work / "libnx-exception-defaults.cpp"
        defaults.write_text("""#include <cstdint>
extern "C" {
alignas(16) __attribute__((weak)) uint8_t __nx_exception_stack[0x400] = {};
__attribute__((weak)) uint64_t __nx_exception_stack_size = sizeof(__nx_exception_stack);
uint64_t LibnxExceptionStackSize() { return __nx_exception_stack_size; }
uint8_t* LibnxExceptionStackAddress() { return __nx_exception_stack; }
}
""")
        executable = work / "exception-contract"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(work),
                str(
                    Path(__file__).resolve().parents[1]
                    / "tests/native_exception_handler_contract.cpp"
                ),
                str(defaults),
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], check=True)
        # Exercise the actual production declaration placement in a unity-style TU.
        bridge = product_changes(reference)["runtime/src/system_bridge.cpp"][1]
        assert bridge.index(SYMBOL_DECLARATIONS) < bridge.index("namespace {\n")
        floor = bridge[
            bridge.index("const char* GuestMapSymbolFloor(") : bridge.index(
                "std::optional<uint32_t> ReadGuest32NoThrow("
            )
        ]
        symbols = """extern "C" {
extern const uint32_t kGuestMapSymbolCount = 2;
extern const uint32_t kGuestMapSymbolAddresses[] = {0x1000, 0x2000};
extern const char* const kGuestMapSymbolNames[] = {"first", "second"};
}
"""
        for order in (0, 1):
            source = work / f"symbols-{order}.cpp"
            declarations = SYMBOL_DECLARATIONS + "namespace {\n" + floor + "}\n"
            source.write_text(
                "#include <cstdint>\n#include <cassert>\n"
                + (symbols + declarations if order else declarations + symbols)
                + """int main() {
uint32_t start = 0;
assert(GuestMapSymbolFloor(0xfff, &start) == nullptr);
assert(GuestMapSymbolFloor(0x1000, &start) == kGuestMapSymbolNames[0] && start == 0x1000);
assert(GuestMapSymbolFloor(0x2001, &start) == kGuestMapSymbolNames[1] && start == 0x2000);
assert(GuestMapSymbolFloor(0x12000, &start) == nullptr);
}
"""
            )
            executable = work / f"symbols-{order}"
            subprocess.run(
                [
                    "g++",
                    "-std=c++20",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    str(source),
                    "-o",
                    str(executable),
                ],
                check=True,
            )
            subprocess.run([str(executable)], check=True)
        # Compile the production automatic-backend list in the native and Android modes.
        aurora = product_changes(reference)["aurora-main/lib/aurora.cpp"][1]
        backend_list = aurora[
            aurora.index(
                "#ifdef AURORA_ENABLE_GX\nconstexpr std::array PreferredBackendOrder{"
            ) : aurora.index("bool g_initialFrame = false;")
        ]
        for target, flags, expected in (
            (
                "switch",
                ["-D__SWITCH__", "-DDAWN_ENABLE_BACKEND_DEKO3D"],
                "static_assert(PreferredBackendOrder.size() == 1 && PreferredBackendOrder[0] == BACKEND_DEKO3D);",
            ),
            (
                "android",
                [
                    "-D__ANDROID__",
                    "-DDAWN_ENABLE_BACKEND_VULKAN",
                    "-DDAWN_ENABLE_BACKEND_OPENGLES",
                ],
                "static_assert(PreferredBackendOrder.size() == 2 && PreferredBackendOrder[0] == BACKEND_VULKAN && PreferredBackendOrder[1] == BACKEND_OPENGLES);",
            ),
        ):
            source = work / (target + ".cpp")
            source.write_text(
                "#include <array>\nenum AuroraBackend {BACKEND_WEBGPU, BACKEND_D3D12, BACKEND_D3D11, BACKEND_METAL, BACKEND_VULKAN, BACKEND_OPENGLES, BACKEND_NULL, BACKEND_DEKO3D};\n"
                + backend_list
                + expected
                + "\nint main() {}\n"
            )
            subprocess.run(
                [
                    "g++",
                    "-std=c++20",
                    "-DAURORA_ENABLE_GX",
                    *flags,
                    str(source),
                    "-o",
                    str(work / target),
                ],
                check=True,
            )
        patch = (
            reference
            / "aurora-main/cmake/patches/sqlite3-nintendo-switch-platform.patch"
        )
        assert (
            digest(patch.read_bytes()) == SQLITE_PATCH
            and len(sqlite_hunks(patch.read_text())) == 3
        )
        invalid = work / "sqlite3.c"
        invalid.write_text("unattributed sqlite source\n")
        try:
            apply_sqlite(invalid, patch)
        except ValueError:
            pass
        else:
            raise AssertionError("unattributed SQLite source accepted")
    print(
        "Product patch guards, exception stack/frame safety, C-linkage/unity and automatic-backend tests passed"
    )


def sqlite_self_test(source, reference):
    """Run the actual pinned SQLite VFS against persistent files and injected read errors."""
    patch = (
        reference / "aurora-main/cmake/patches/sqlite3-nintendo-switch-platform.patch"
    )
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        copied = work / "sqlite3.c"
        copied.write_bytes(source.read_bytes())
        apply_sqlite(copied, patch)
        before = copied.read_bytes()
        apply_sqlite(copied, patch)
        apply_sqlite(copied, patch, check=True)
        assert copied.read_bytes() == before
        (work / "sqlite3.h").write_bytes((source.parent / "sqlite3.h").read_bytes())
        (work / "test.c").write_text(r"""#include "sqlite3.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int real_error = 0;
static unsigned eof_errors = 0;
static ssize_t horizon_read(int fd, void *buffer, size_t size, off_t offset) {
  struct stat info;
  if (fstat(fd, &info) == 0 && offset >= info.st_size && size) {
    ++eof_errors; errno = EIO; return -1;
  }
  if (real_error) { errno = EIO; return -1; }
  return pread(fd, buffer, size, offset);
}
int main(void) {
  sqlite3_vfs *vfs = sqlite3_vfs_find("unix-none");
  assert(vfs && vfs->xSetSystemCall && vfs->xFullPathname);
  char path[1024];
  assert(vfs->xFullPathname(vfs, "sdmc:/pipeline.db", sizeof(path), path) == SQLITE_OK);
  assert(strcmp(path, "sdmc:/pipeline.db") == 0);
  char short_path[8];
  memset(short_path, 'x', sizeof(short_path));
  assert(vfs->xFullPathname(vfs, "sdmc:/pipeline.db", sizeof(short_path), short_path) != SQLITE_OK);
  assert(mkdir("sdmc:", 0700) == 0);
  assert(vfs->xSetSystemCall(vfs, "pread", (sqlite3_syscall_ptr)horizon_read) == SQLITE_OK);
  assert(vfs->xSetSystemCall(vfs, "pread64", (sqlite3_syscall_ptr)horizon_read) == SQLITE_OK);
  sqlite3 *db = 0;
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  assert(sqlite3_open_v2("sdmc:/pipeline.db", &db, flags, "unix-none") == SQLITE_OK);
  assert(sqlite3_exec(db, "CREATE TABLE cache(value INTEGER); INSERT INTO cache VALUES(42);", 0, 0, 0) == SQLITE_OK);
  assert(sqlite3_close(db) == SQLITE_OK);
  assert(eof_errors > 0);
  assert(sqlite3_open_v2("sdmc:/pipeline.db", &db, flags, "unix-none") == SQLITE_OK);
  sqlite3_stmt *stmt = 0;
  assert(sqlite3_prepare_v2(db, "SELECT value FROM cache", -1, &stmt, 0) == SQLITE_OK);
  assert(sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 42);
  assert(sqlite3_finalize(stmt) == SQLITE_OK);
  assert(sqlite3_close(db) == SQLITE_OK);
  real_error = 1;
  int result = sqlite3_open_v2("sdmc:/pipeline.db", &db, flags, "unix-none");
  if (result == SQLITE_OK) result = sqlite3_exec(db, "SELECT value FROM cache", 0, 0, 0);
  assert((result & 255) == SQLITE_IOERR);
  sqlite3_close(db);
  return 0;
}
""")
        executable = work / "sqlite-test"
        # Poison unsupported function names: any live ownership reference fails the link.
        subprocess.run(
            [
                "cc",
                "-std=c11",
                "-D_GNU_SOURCE",
                "-D__SWITCH__",
                "-DSQLITE_MAX_MMAP_SIZE=0",
                "-DSQLITE_OMIT_WAL",
                "-DSQLITE_OMIT_LOAD_EXTENSION",
                '-DSQLITE_DEFAULT_UNIX_VFS="unix-none"',
                "-Dfchown=MKW_FORBIDDEN_fchown",
                "-Dgeteuid=MKW_FORBIDDEN_geteuid",
                str(copied),
                str(work / "test.c"),
                "-lpthread",
                "-lm",
                "-o",
                str(executable),
            ],
            check=True,
        )
        subprocess.run([str(executable)], cwd=work, check=True)
    print(
        "Pinned SQLite persistence, device paths, EOF/EIO and ownership-link tests passed"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("workspace", nargs="?", type=Path)
    parser.add_argument(
        "--reference",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "third_party/DriftDroid",
    )
    parser.add_argument("--sqlite", type=Path)
    parser.add_argument("--sqlite-patch", type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--sqlite-self-test", type=Path)
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test(args.reference)
        elif args.sqlite_self_test is not None:
            sqlite_self_test(args.sqlite_self_test, args.reference)
        elif args.sqlite is not None:
            if args.sqlite_patch is None:
                parser.error("--sqlite requires --sqlite-patch")
            apply_sqlite(args.sqlite, args.sqlite_patch, args.check)
            print("Complete SQLite Switch portability hash verified")
        elif args.workspace is not None:
            apply_product(args.workspace, args.reference, args.check)
            print("Native product source portability contract verified")
        else:
            parser.error("workspace, --sqlite or --self-test is required")
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"error: {error}") from error


if __name__ == "__main__":
    main()
