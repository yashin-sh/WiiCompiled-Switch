#!/usr/bin/env python3
"""Prepare Switch-adapted runtime headers in a replay build mirror."""

import argparse
import importlib.util
import io
import subprocess
import tarfile
import tempfile
from pathlib import Path

WII_PIN = "a135beb201042b20f390c6695ca6b26768820fb4"
PATHS = (
    "runtime/include",
    "runtime/src/hle/gx",
    "aurora-main/include/dolphin/gx/GXGeometry.h",
)


def prepare(checkout: Path, destination: Path) -> None:
    scripts = Path(__file__).resolve().parent
    spec = importlib.util.spec_from_file_location(
        "replay_aurora", scripts / "prepare-replay-aurora.py"
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    git = module.git_args(checkout)
    actual = subprocess.check_output([*git, "rev-parse", "HEAD"], text=True).strip()
    if actual != WII_PIN:
        raise ValueError(f"replay runtime pin mismatch: {actual}")
    archive = subprocess.check_output([*git, "archive", WII_PIN, *PATHS])
    # Read original pinned blobs, never the user's modified integration headers.
    # The existing public patch is applied only in a standalone temporary tree.
    with tempfile.TemporaryDirectory(prefix="wc-replay-runtime-") as temporary:
        stage = Path(temporary)
        with tarfile.open(fileobj=io.BytesIO(archive)) as source:
            for member in source.getmembers():
                if member.isdir():
                    continue
                relative = Path(member.name)
                if (
                    not member.isfile()
                    or relative.is_absolute()
                    or ".." in relative.parts
                ):
                    raise ValueError(f"unexpected pinned runtime member: {member.name}")
                target = stage / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.extractfile(member).read())
        patch = scripts.parent / "patches/wiicompiled/m3-wiicompiled-switch-build.patch"
        subprocess.run(["git", "apply", "--check", str(patch)], cwd=stage, check=True)
        subprocess.run(["git", "apply", str(patch)], cwd=stage, check=True)
        for source in stage.rglob("*"):
            if not source.is_file():
                continue
            target = destination / source.relative_to(stage)
            data = source.read_bytes()
            if not target.exists() or target.read_bytes() != data:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
    print("PASS: replay runtime mirror from pinned blobs and public Switch patch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkout", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    prepare(args.checkout.resolve(), args.destination.resolve())


if __name__ == "__main__":
    main()
