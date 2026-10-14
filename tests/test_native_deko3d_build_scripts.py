import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = (ROOT / "scripts/build-local-native-deko3d.sh").read_text()
LOOP = SCRIPT[
    SCRIPT.index("dawn_submodule_status=") : SCRIPT.index(
        "# The separate DriftDroid Abseil patch"
    )
]


class Submodules(unittest.TestCase):
    def run_fixture(self, status, dirty="", staged="", untracked=""):
        with tempfile.TemporaryDirectory() as directory:
            fixture = Path(directory)
            git = fixture / "git"
            git.write_text("""#!/bin/bash
set -eu
path="$2"
shift 2
case "$1" in
  submodule) printf '%s\\n' "$FIXTURE_STATUS" ;;
  diff)
    if [[ "$*" == *--cached* ]]; then
      [[ "$path" != "/dawn/$FIXTURE_STAGED" ]]
    else
      [[ "$path" != "/dawn/$FIXTURE_DIRTY" ]]
    fi ;;
  ls-files) [[ "$path" != "/dawn/$FIXTURE_UNTRACKED" ]] || echo unexpected.cpp ;;
  *) exit 98 ;;
esac
""")
            git.chmod(0o755)
            env = dict(
                os.environ,
                PATH=f"{fixture}:" + os.environ["PATH"],
                FIXTURE_STATUS=status,
                FIXTURE_DIRTY=dirty,
                FIXTURE_STAGED=staged,
                FIXTURE_UNTRACKED=untracked,
            )
            shell = (
                "set -euo pipefail\nDAWN_SOURCE_DIR=/dawn\nDAWN_EXCLUDE_ABSEIL=third_party/abseil-cpp\n"
                + LOOP
            )
            return subprocess.run(
                ["bash", "-c", shell],
                env=env,
                capture_output=True,
                text=True,
                check=False,
            )

    def test_only_intentional_skips_allowed(self):
        for path in ("third_party/angle", "third_party/swiftshader"):
            with self.subTest(path=path):
                self.assertEqual(self.run_fixture(f"-{'a' * 40} {path}").returncode, 0)
        self.assertEqual(
            self.run_fixture(f"-{'a' * 40} third_party/zlib").returncode, 3
        )

    def test_moved_and_conflicted_skips_rejected(self):
        for prefix in "+U":
            for path in (
                "third_party/angle",
                "third_party/swiftshader",
                "third_party/zlib",
            ):
                with self.subTest(prefix=prefix, path=path):
                    self.assertEqual(
                        self.run_fixture(f"{prefix}{'a' * 40} {path}").returncode, 3
                    )

    def test_initialized_submodules_audited(self):
        for path in (
            "third_party/angle",
            "third_party/swiftshader",
            "third_party/zlib",
            "third_party/foo/nested",
        ):
            status = f" {'a' * 40} {path}"
            self.assertEqual(self.run_fixture(status).returncode, 0)
            for kind in ("dirty", "staged", "untracked"):
                with self.subTest(path=path, kind=kind):
                    self.assertEqual(
                        self.run_fixture(status, **{kind: path}).returncode, 3
                    )

    def test_audited_abseil_delta_allowed(self):
        path = "third_party/abseil-cpp"
        self.assertEqual(
            self.run_fixture(f" {'a' * 40} {path}", dirty=path).returncode, 0
        )

    def test_workspace_defaults_match_custom_deps(self):
        prepare = (ROOT / "scripts/prepare-local-native-deko3d-product.sh").read_text()
        for script in (prepare, SCRIPT):
            variables = "\n".join(
                line
                for line in script.splitlines()
                if line.startswith(("readonly DEPS_DIR=", "readonly WORK_DIR="))
            )
            for override in ("", "/custom/workspace"):
                env = dict(
                    os.environ,
                    MKW_DEKO3D_DEPS_DIR="/custom/deps",
                    MKW_DEKO3D_WORK_DIR=override,
                )
                result = subprocess.run(
                    [
                        "bash",
                        "-c",
                        "ROOT_DIR=/repo\n" + variables + '\nprintf "%s" "$WORK_DIR"',
                    ],
                    env=env,
                    capture_output=True,
                    text=True,
                    check=True,
                )
                self.assertEqual(result.stdout, override or "/custom/deps/driftdroid")


class ProvenanceAndPaths(unittest.TestCase):
    @staticmethod
    def block(script, marker):
        tail = script[script.index(marker) :]
        return tail.split("<<'PY'\n", 1)[1].split("\nPY", 1)[0]

    def run_block(self, source, *args):
        return subprocess.run(
            ["python3", "-c", source, *map(str, args)],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_dependency_receipt_requires_matching_unique_fields(self):
        source = self.block(SCRIPT, 'python3 - "$DEP_RECEIPT"')
        with tempfile.TemporaryDirectory() as directory:
            receipt = Path(directory) / "receipt"
            receipt.write_text("dawn_archive_sha256=expected\n")
            self.assertEqual(
                self.run_block(
                    source, receipt, "dawn_archive_sha256=expected"
                ).returncode,
                0,
            )
            self.assertNotEqual(
                self.run_block(
                    source, receipt, "dawn_archive_sha256=changed"
                ).returncode,
                0,
            )
            self.assertNotEqual(
                self.run_block(source, receipt, "uam_commit=missing").returncode, 0
            )
            receipt.write_text(
                "dawn_archive_sha256=expected\ndawn_archive_sha256=expected\n"
            )
            self.assertNotEqual(
                self.run_block(
                    source, receipt, "dawn_archive_sha256=expected"
                ).returncode,
                0,
            )

    def test_translation_receipt_rejects_wrong_pin_or_inputs(self):
        import hashlib

        source = self.block(
            SCRIPT, 'python3 - "$WORK_DIR/.wcs-native-deko3d-provenance"'
        )
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            original, copied = folder / "original", folder / "copied"
            original.mkdir()
            copied.mkdir()
            fields = ["reference_commit=pin"]
            for name, key in [
                ("main.dol", "main_dol_sha256"),
                ("StaticR.rel", "staticr_rel_sha256"),
            ]:
                data = name.encode()
                (original / name).write_bytes(data)
                (copied / name).write_bytes(data)
                fields.append(f"{key}={hashlib.sha256(data).hexdigest()}")
            receipt = folder / "receipt"
            receipt.write_text("\n".join(fields))
            self.assertEqual(
                self.run_block(source, receipt, "pin", original, copied).returncode, 0
            )
            self.assertNotEqual(
                self.run_block(
                    source, receipt, "other-pin", original, copied
                ).returncode,
                0,
            )
            for changed in (original, copied):
                (changed / "main.dol").write_bytes(b"changed")
                self.assertNotEqual(
                    self.run_block(source, receipt, "pin", original, copied).returncode,
                    0,
                )
                (changed / "main.dol").write_bytes(b"main.dol")
            receipt.unlink()
            self.assertNotEqual(
                self.run_block(source, receipt, "pin", original, copied).returncode, 0
            )

    def test_cleanup_guards_protected_and_public_paths(self):
        prepare = (ROOT / "scripts/prepare-local-native-deko3d-product.sh").read_text()
        source = self.block(prepare, 'python3 - "$WORK_DIR"')
        assets, reference, deps, output = (
            ROOT / "local-product/Assets",
            ROOT / "third_party/DriftDroid",
            ROOT / ".deps/native-deko3d",
            ROOT / "local-product/native-deko3d",
        )
        for work in (
            ROOT,
            ROOT.parent,
            assets,
            assets / "generated",
            reference,
            reference / "generated",
            deps,
            output,
            ROOT / "public-generated",
        ):
            with self.subTest(work=work):
                self.assertNotEqual(
                    self.run_block(
                        source, work, ROOT, assets, reference, deps, output
                    ).returncode,
                    0,
                )
        self.assertEqual(
            self.run_block(
                source, deps / "driftdroid", ROOT, assets, reference, deps, output
            ).returncode,
            0,
        )
        with tempfile.TemporaryDirectory() as directory:
            alias = Path(directory) / "workspace"
            alias.symlink_to(assets, target_is_directory=True)
            self.assertNotEqual(
                self.run_block(
                    source, alias, ROOT, assets, reference, deps, output
                ).returncode,
                0,
            )


if __name__ == "__main__":
    unittest.main()
