#!/usr/bin/env python3
"""Keep the replay job present while running its full suite for relevant changes."""

import argparse
import fnmatch
import subprocess

PATTERNS = (
    "desktop-gx-replay/**",
    "include/frame_dump*",
    "include/rendered_frame_dump.hpp",
    "include/surface_presenter.hpp",
    "source/frame_dump*",
    "source/nw4r_lyt_draw_quad_hle_bridge.cpp",
    "tests/lyt_draw_quad_contract.cpp",
    "scripts/test-lyt-draw-quad.sh",
    "source/rendered_frame_dump.cpp",
    "source/surface_presenter.cpp",
    "source/rendered_fifo_capture.cpp",
    "tests/frame_dump*",
    "tests/fifo_capture_contract.cpp",
    "scripts/test-frame-dump.sh",
    "scripts/*desktop-gx-replay.sh",
    "scripts/prepare-replay-aurora.py",
    "scripts/prepare-checked-aurora-fifo.py",
    "scripts/ci_desktop_replay_needed.py",
    ".github/workflows/desktop-replay.yml",
    "third_party/WiiCompiled",
)


def needs_replay(names):
    return any(
        fnmatch.fnmatchcase(name, pattern) for name in names for pattern in PATTERNS
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args()
    if not args.base or set(args.base) == {"0"}:
        needed = True
    else:
        names = (
            subprocess.check_output(
                ["git", "diff", "--name-only", "-z", f"{args.base}...{args.head}"],
            )
            .decode()
            .split("\0")
        )
        needed = needs_replay(names)
    print(f"needed={'true' if needed else 'false'}")


if __name__ == "__main__":
    main()
