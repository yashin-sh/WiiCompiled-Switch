"""Apply the minimal libnx portability fixes required by Dawn's pinned Abseil.

This intentionally patches only two known incompatibilities in the exact Dawn
submodule used by the native Switch/Deko3D path:
  * libnx pthread_t is an opaque pointer, so GetTID() must use Horizon's thread handle.
  * libnx pthreads do not expose POSIX signal-mask APIs, so thread identity setup
    must use the existing no-signals pthread_setspecific path.

The transformation is deliberately strict and idempotent. If upstream text no
longer matches the pinned source contract, it fails instead of guessing.
"""

import argparse
from pathlib import Path

SYSINFO_INCLUDE_ANCHOR = "#ifdef __linux__\n#include <sys/syscall.h>\n#endif\n"
SYSINFO_SWITCH_INCLUDE = "#ifdef __SWITCH__\n#include <switch.h>\n#endif\n"
SYSINFO_FALLBACK = """#else

// Fallback implementation of `GetTID` using `pthread_self`.
pid_t GetTID() {
  // `pthread_t` need not be arithmetic per POSIX; platforms where it isn't
  // should be handled above.
  return static_cast<pid_t>(pthread_self());
}
"""
SYSINFO_SWITCH_BRANCH = """#elif defined(__SWITCH__)

pid_t GetTID() { return static_cast<pid_t>(threadGetCurHandle()); }

#else

// Fallback implementation of `GetTID` using `pthread_self`.
pid_t GetTID() {
  // `pthread_t` need not be arithmetic per POSIX; platforms where it isn't
  // should be handled above.
  return static_cast<pid_t>(pthread_self());
}
"""

THREAD_IDENTITY_PATTERNS = (
    (
        "#if defined(__wasi__) || defined(__EMSCRIPTEN__) || defined(__MINGW32__) || \\\n    defined(__hexagon__)",
        "#if defined(__wasi__) || defined(__EMSCRIPTEN__) || defined(__MINGW32__) || \\\n    defined(__hexagon__) || defined(__SWITCH__)",
    ),
    (
        "#if defined(__EMSCRIPTEN__) || defined(__MINGW32__)",
        "#if defined(__EMSCRIPTEN__) || defined(__MINGW32__) || defined(__SWITCH__)",
    ),
)


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def patch_sysinfo(path: Path, check_only: bool) -> bool:
    text = path.read_text(encoding="utf-8")
    changed = False

    if SYSINFO_SWITCH_INCLUDE not in text:
        if check_only:
            fail(f"Switch include missing from {path}")
        if SYSINFO_INCLUDE_ANCHOR not in text:
            fail(f"sysinfo include anchor changed in {path}")
        text = text.replace(
            SYSINFO_INCLUDE_ANCHOR,
            SYSINFO_INCLUDE_ANCHOR + "\n" + SYSINFO_SWITCH_INCLUDE,
            1,
        )
        changed = True

    if "#elif defined(__SWITCH__)\n\npid_t GetTID()" not in text:
        if check_only:
            fail(f"Switch GetTID branch missing from {path}")
        if SYSINFO_FALLBACK not in text:
            fail(f"sysinfo GetTID fallback contract changed in {path}")
        text = text.replace(SYSINFO_FALLBACK, SYSINFO_SWITCH_BRANCH, 1)
        changed = True

    if changed:
        path.write_text(text, encoding="utf-8")
    return changed


def patch_thread_identity(path: Path, check_only: bool) -> bool:
    text = path.read_text(encoding="utf-8")

    marker = "defined(__SWITCH__)"
    signal_call = "pthread_sigmask(SIG_SETMASK, &all_signals, &curr_signals);"
    if marker in text and signal_call in text:
        # The source can contain unrelated Switch conditionals in future; require
        # the no-signals branch itself to mention Switch.
        idx = text.find(signal_call)
        branch_start = text.rfind("#if", 0, idx)
        branch_prefix = text[branch_start:idx] if branch_start >= 0 else ""
        if marker in branch_prefix:
            return False

    if check_only:
        fail(f"Switch no-signals pthread branch missing from {path}")

    for old, new in THREAD_IDENTITY_PATTERNS:
        if old in text:
            text = text.replace(old, new, 1)
            path.write_text(text, encoding="utf-8")
            return True

    fail(f"thread_identity no-signals condition changed in {path}")
    return False


def verify(root: Path) -> None:
    sysinfo = root / "absl/base/internal/sysinfo.cc"
    thread_identity = root / "absl/base/internal/thread_identity.cc"
    if not sysinfo.is_file() or not thread_identity.is_file():
        fail(f"not an initialized Abseil checkout: {root}")
    patch_sysinfo(sysinfo, check_only=True)
    patch_thread_identity(thread_identity, check_only=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("abseil_root", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    root = args.abseil_root.resolve()
    sysinfo = root / "absl/base/internal/sysinfo.cc"
    thread_identity = root / "absl/base/internal/thread_identity.cc"
    if not sysinfo.is_file() or not thread_identity.is_file():
        fail(f"not an initialized Abseil checkout: {root}")

    if args.check:
        verify(root)
        print(f"Dawn Abseil Switch portability contract OK: {root}")
        return

    changed = False
    changed |= patch_sysinfo(sysinfo, check_only=False)
    changed |= patch_thread_identity(thread_identity, check_only=False)
    verify(root)
    status = "applied" if changed else "already present"
    print(f"Dawn Abseil Switch portability {status}: {root}")


if __name__ == "__main__":
    main()
