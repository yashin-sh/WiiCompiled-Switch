"""Network contracts against the production C++ server, compiled by test-mkw-debug.sh."""

import hashlib
import os
import select
import socket
import subprocess
import sys
import tempfile
import threading
import unittest
import zlib
from pathlib import Path
from unittest.mock import patch

from tools import mkw_debug

ROOT = Path(__file__).resolve().parents[1]
HOST_BINARY = os.environ.get("MKW_DEBUG_HOST_TEST_BINARY")


def nro(payload):
    """Small public fixture with a structurally valid NRO header, no game data."""
    content = bytearray(128)
    content[16:20] = b"NRO0"
    content.extend(payload)
    content[24:28] = len(content).to_bytes(4, "little")
    return bytes(content)


def frame(stream):
    header = stream.readline(129)
    if not header:
        raise EOFError("server closed connection")
    if not header.endswith(b"\n"):
        raise AssertionError(f"incomplete response header: {header!r}")
    status, count = header.rstrip(b"\n").split(b" ")
    if status not in (b"ok", b"err"):
        raise AssertionError(f"invalid response header: {header!r}")
    count = int(count)
    if not 0 <= count <= 512 * 1024 * 1024:
        raise AssertionError(f"invalid response size: {count}")
    payload = bytearray()
    while len(payload) < count:
        chunk = stream.read(count - len(payload))
        if not chunk:
            raise AssertionError("truncated response")
        payload.extend(chunk)
    return status, bytes(payload)


@unittest.skipUnless(
    HOST_BINARY, "run scripts/test-mkw-debug.sh for the C++ network server"
)
class DebugServerContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.base = Path(self.temp.name)
        self.sd = self.base / "WiiCompiled-Switch"
        self.sd.mkdir()
        (self.sd / "reports").mkdir()
        (self.sd / "logs").mkdir()
        (self.sd / "captures").mkdir()
        (self.sd / "reports" / "dispatch.txt").write_bytes(b"dispatch=608214\n")
        (self.sd / "logs" / "session.log").write_bytes(b"boot\n")
        (self.sd / "captures" / "frame.png").write_bytes(b"\x89PNG\r\n\x1a\nfixture")
        self.original = b"old NRO candidate\x00" * 257
        (self.sd / "WiiCompiled-Switch.nro").write_bytes(self.original)
        self.process = None
        self.connections = []
        self.start_server()

    def start_server(self, enabled=True):
        self.process = subprocess.Popen(
            [HOST_BINARY, str(self.sd), "enabled" if enabled else "disabled"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        readable, _, _ = select.select([self.process.stdout], [], [], 8)
        if not readable:
            self.fail("host server did not start")
        line = self.process.stdout.readline()
        self.assertTrue(line.startswith(b"PORT "), line)
        self.port = int(line.split()[1])

    def stop_server(self):
        if self.process is None:
            return
        self.process.stdin.write(b"STOP\n")
        self.process.stdin.flush()
        try:
            out, err = self.process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill()
            out, err = self.process.communicate()
            self.fail(f"server stop hung with active connection: {out!r} {err!r}")
        self.assertEqual(self.process.returncode, 0, err.decode(errors="replace"))
        self.assertIn(b"STOPPED", out)
        self.assertNotIn(b"runtime error:", err)
        self.assertNotIn(b"ERROR: AddressSanitizer", err)
        self.process = None

    def tearDown(self):
        for stream, conn in self.connections:
            stream.close()
            conn.close()
        try:
            self.stop_server()
        finally:
            self.temp.cleanup()

    def connect(self):
        conn = socket.create_connection(("127.0.0.1", self.port), timeout=3)
        stream = conn.makefile("rb")
        self.connections.append((stream, conn))
        return conn, stream

    def request(self, command):
        conn, stream = self.connect()
        conn.sendall(command.encode() + b"\n")
        status, payload = frame(stream)
        stream.close()
        conn.close()
        self.connections.remove((stream, conn))
        return status, payload

    def log(self, payload):
        self.process.stdin.write(b"LOG " + payload + b"\n")
        self.process.stdin.flush()
        self.assertEqual(self.process.stdout.readline(), b"DONE\n")

    def fault(self, name):
        self.process.stdin.write(name.encode() + b"\n")
        self.process.stdin.flush()
        self.assertEqual(self.process.stdout.readline(), b"DONE\n")

    def cli(self, *arguments, check=True):
        result = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/mkw_debug.py"),
                "--host",
                "127.0.0.1",
                "--port",
                str(self.port),
                "--timeout",
                "3",
                *map(str, arguments),
            ],
            capture_output=True,
            timeout=15,
            check=False,
        )
        if check:
            self.assertEqual(
                result.returncode, 0, result.stderr.decode(errors="replace")
            )
        return result

    def upload(self, body, checksum=None):
        conn, stream = self.connect()
        crc = zlib.crc32(body) if checksum is None else checksum
        conn.sendall(f"deploy {len(body)} {crc:08x}\n".encode())
        self.assertEqual(frame(stream), (b"ok", b""))
        conn.sendall(body)
        result = frame(stream)
        stream.close()
        conn.close()
        self.connections.remove((stream, conn))
        return result

    def test_ping_and_live_runtime_info(self):
        self.assertEqual(self.request("ping")[0], b"ok")
        status, payload = self.request("info")
        self.assertEqual(status, b"ok")
        fields = dict(
            line.split("=", 1) for line in payload.decode().splitlines() if "=" in line
        )
        self.assertEqual(fields["dispatch"], "608214")
        self.assertEqual(fields["guest_pc"].lower(), "0x8019b964")
        self.assertEqual(fields["frontier"], "NANDSeek")
        self.assertEqual(fields["stage"], "host-contract")
        self.assertGreaterEqual(float(fields["uptime"]), 0)
        self.assertGreater(float(fields["fps"]), 0)

    def test_default_configuration_does_not_listen(self):
        self.stop_server()
        self.start_server(enabled=False)
        self.assertEqual(self.port, 0)

    def test_bad_commands_are_errors_and_server_recovers(self):
        for command in (
            "unknown",
            "get",
            "deploy -1 12345678",
            "deploy 536870913 00000000",
            "deploy 3 nonsense",
            "get ../secret",
            "get /etc/passwd",
        ):
            with self.subTest(command=command):
                self.assertEqual(self.request(command)[0], b"err")
        self.assertEqual(self.request("ping")[0], b"ok")

    def test_overlong_and_nul_commands_do_not_escape_confinement(self):
        for line in (
            b"x" * 4096 + b"\n",
            b"get reports/dispatch.txt\x00../../secret\n",
        ):
            conn, stream = self.connect()
            conn.sendall(line)
            try:
                status, _ = frame(stream)
                self.assertEqual(status, b"err")
            except (ConnectionResetError, EOFError):
                # Closing a malformed-command connection is also a valid rejection.
                pass
            stream.close()
            conn.close()
            self.connections.remove((stream, conn))
        self.assertEqual(self.request("ping")[0], b"ok")

    def test_report_listing_download_and_symlink_confinement(self):
        secret = self.base / "secret.txt"
        secret.write_bytes(b"private outside root")
        (self.sd / "reports" / "escape.txt").symlink_to(secret)
        (self.sd / "logs" / "escape").symlink_to(self.base, target_is_directory=True)
        status, listing = self.request("reports")
        self.assertEqual(status, b"ok")
        self.assertIn(b"reports/dispatch.txt", listing)
        self.assertNotIn(b"escape", listing)
        self.assertEqual(
            self.request("get reports/dispatch.txt"), (b"ok", b"dispatch=608214\n")
        )
        self.assertEqual(self.request("get reports/escape.txt")[0], b"err")
        self.assertEqual(self.request("get logs/escape/secret.txt")[0], b"err")
        self.assertEqual(self.request("get WiiCompiled-Switch.nro")[0], b"err")

    def test_interrupted_upload_leaves_original_and_removes_part(self):
        conn, stream = self.connect()
        conn.sendall(b"deploy 4096 00000000\n")
        self.assertEqual(frame(stream), (b"ok", b""))
        conn.sendall(b"partial upload")
        conn.shutdown(socket.SHUT_RDWR)
        stream.close()
        conn.close()
        self.connections.remove((stream, conn))
        self.assertEqual(self.request("ping")[0], b"ok")
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), self.original
        )
        self.assertFalse(list(self.sd.glob("*.part")))

    def test_crc_mismatch_preserves_original(self):
        self.assertEqual(self.upload(nro(b"corrupt candidate"), 0)[0], b"err")
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), self.original
        )
        self.assertFalse(list(self.sd.glob("*.part")))

    def test_valid_crc_cannot_commit_an_invalid_nro_header(self):
        self.assertEqual(self.upload(b"not an NRO" * 128)[0], b"err")
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), self.original
        )
        self.assertFalse(list(self.sd.glob("*.part")))

    def test_cleanup_requires_size_and_hash_and_preserves_current_and_foreign_files(
        self,
    ):
        old = self.sd / "WiiCompiled-Switch-old.nro"
        old.write_bytes(b"older candidate")
        size = old.stat().st_size
        sha = hashlib.sha256(old.read_bytes()).hexdigest()
        self.assertEqual(
            self.request(f"remove-nro {old.name} {size + 1} {sha}")[0], b"err"
        )
        self.assertEqual(
            self.request(f"remove-nro {old.name} {size} {'0' * 64}")[0], b"err"
        )
        self.assertTrue(old.exists())
        current_sha = hashlib.sha256(self.original).hexdigest()
        self.assertEqual(
            self.request(
                f"remove-nro WiiCompiled-Switch.nro {len(self.original)} {current_sha}"
            )[0],
            b"err",
        )
        foreign = self.sd / "another-app.nro"
        foreign.write_bytes(b"other application")
        self.assertEqual(self.request("get-nro another-app.nro")[0], b"err")
        self.assertEqual(self.request(f"remove-nro {old.name} {size} {sha}")[0], b"ok")
        self.assertFalse(old.exists())
        self.assertTrue(foreign.exists())

    def test_valid_upload_retains_rollback_after_complete_readback(self):
        candidate = nro(bytes(range(256)) * 1024)
        status, metadata = self.upload(candidate)
        self.assertEqual(status, b"ok", metadata)
        self.assertIn(hashlib.sha256(candidate).hexdigest().encode(), metadata)
        self.assertEqual((self.sd / "WiiCompiled-Switch.nro").read_bytes(), candidate)
        self.assertEqual(self.request("get-nro"), (b"ok", candidate))
        self.assertFalse(list(self.sd.glob("*.part")))
        rollback = self.sd / "WiiCompiled-Switch.nro.rollback"
        self.assertEqual(rollback.read_bytes(), self.original)
        self.assertEqual(
            self.request("get-nro WiiCompiled-Switch.nro.rollback"),
            (b"ok", self.original),
        )
        # A complete readback retains the previous candidate until a guarded
        # finalization confirms both the new file and the backed-up old file.
        self.assertTrue(rollback.exists())

    def test_failed_commit_rename_restores_previous_candidate(self):
        self.fault("FAIL_COMMIT")
        self.assertEqual(self.upload(nro(b"new candidate"))[0], b"err")
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), self.original
        )
        self.assertFalse((self.sd / "WiiCompiled-Switch.nro.rollback").exists())
        self.assertFalse(list(self.sd.glob("*.part")))
        self.assertEqual(self.request("ping")[0], b"ok")

    def test_restart_recovers_power_loss_between_rotation_and_commit(self):
        candidate = nro(b"new candidate")
        self.fault("CRASH_COMMIT")
        with self.assertRaises((EOFError, ConnectionResetError)):
            self.upload(candidate)
        _, err = self.process.communicate(timeout=5)
        self.assertEqual(self.process.returncode, 90, err.decode(errors="replace"))
        self.process = None
        self.assertFalse((self.sd / "WiiCompiled-Switch.nro").exists())
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro.rollback").read_bytes(), self.original
        )
        self.start_server()
        self.assertEqual(self.request("get-nro"), (b"ok", self.original))
        self.assertFalse((self.sd / "WiiCompiled-Switch.nro.rollback").exists())
        # A later deploy can safely replace any stale uncommitted .part.
        self.assertEqual(self.upload(candidate)[0], b"ok")

    def test_finalize_requires_exact_new_readback_and_old_backup_metadata(self):
        candidate = nro(b"new candidate")
        self.assertEqual(self.upload(candidate)[0], b"ok")
        rollback = self.sd / "WiiCompiled-Switch.nro.rollback"
        sha = hashlib.sha256(candidate).hexdigest()
        old_sha = hashlib.sha256(self.original).hexdigest()
        cases = (
            f"{len(candidate) + 1} {sha} {len(self.original)} {old_sha}",
            f"{len(candidate)} {'0' * 64} {len(self.original)} {old_sha}",
            f"{len(candidate)} {sha} {len(self.original) + 1} {old_sha}",
            f"{len(candidate)} {sha} {len(self.original)} {'0' * 64}",
            f"{len(candidate)} {sha} 0 -",
        )
        for arguments in cases:
            with self.subTest(arguments=arguments):
                self.assertEqual(
                    self.request("finalize-deploy " + arguments)[0], b"err"
                )
                self.assertEqual(rollback.read_bytes(), self.original)
        self.assertEqual(self.request("get-nro"), (b"ok", candidate))
        self.assertTrue(rollback.exists())
        self.assertEqual(
            self.request(
                f"finalize-deploy {len(candidate)} {sha} {len(self.original)} {old_sha}"
            )[0],
            b"ok",
        )
        self.assertFalse(rollback.exists())
        self.assertEqual((self.sd / "WiiCompiled-Switch.nro").read_bytes(), candidate)

    def test_pending_rollback_blocks_another_upload_until_verified_finalization(self):
        first = nro(b"first candidate")
        self.assertEqual(self.upload(first)[0], b"ok")
        self.assertEqual(self.request("deploy 4096 00000000")[0], b"err")
        self.assertEqual((self.sd / "WiiCompiled-Switch.nro").read_bytes(), first)
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro.rollback").read_bytes(), self.original
        )

    def test_client_resumes_a_committed_deployment_with_pending_rollback(self):
        first = nro(b"previous unfinalized candidate")
        self.assertEqual(self.upload(first)[0], b"ok")
        self.stop_server()
        self.start_server()
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro.rollback").read_bytes(), self.original
        )
        candidate = self.base / "built.nro"
        candidate.write_bytes(nro(b"next candidate"))
        backups = self.base / "resumed-backups"
        self.cli("deploy", candidate, "--backup-dir", backups)
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), candidate.read_bytes()
        )
        self.assertFalse((self.sd / "WiiCompiled-Switch.nro.rollback").exists())
        self.assertEqual((backups / "WiiCompiled-Switch.nro").read_bytes(), first)
        self.assertEqual(
            (backups / "WiiCompiled-Switch.nro.rollback").read_bytes(), self.original
        )

    def test_logs_replay_history_follow_live_and_allow_reconnect(self):
        self.log(b"before subscription")
        conn, stream = self.connect()
        conn.sendall(b"log\n")
        self.assertEqual(frame(stream), (b"ok", b""))
        history = bytearray()
        for _ in range(16):
            history.extend(frame(stream)[1])
            if b"before subscription" in history:
                break
        self.assertIn(b"before subscription", history)
        self.assertIn(b"stage=host-contract", history)
        self.assertIn(b"dispatch=608214", history)
        self.assertIn(b"frontier=NANDSeek", history)
        self.log(b"after subscription")
        self.assertIn(b"after subscription", frame(stream)[1])
        self.assertEqual(self.request("ping")[0], b"ok")
        self.assertEqual(self.request("info")[0], b"ok")
        self.assertEqual(self.upload(nro(b"deploy with active logs"))[0], b"ok")
        stream.close()
        conn.close()
        self.connections.remove((stream, conn))
        self.assertEqual(self.request("ping")[0], b"ok")

    def test_slow_log_reader_does_not_block_producer_or_shutdown(self):
        conn, stream = self.connect()
        conn.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        conn.sendall(b"log\n")
        self.assertEqual(frame(stream), (b"ok", b""))
        self.process.stdin.write(b"FLOOD\n")
        self.process.stdin.flush()
        readable, _, _ = select.select([self.process.stdout], [], [], 5)
        self.assertTrue(readable, "log producer blocked behind a slow subscriber")
        self.assertEqual(self.process.stdout.readline(), b"DONE\n")
        self.assertEqual(self.request("ping")[0], b"ok")
        self.stop_server()

    def test_quit_closes_only_the_command_session(self):
        self.assertEqual(self.request("quit")[0], b"ok")
        self.assertEqual(self.request("ping")[0], b"ok")

    def test_host_reload_is_rejected_explicitly(self):
        self.assertEqual(self.request("reload")[0], b"err")

    def test_clean_shutdown_interrupts_partial_command(self):
        conn, _ = self.connect()
        conn.sendall(b"unfinished command")
        self.stop_server()

    def test_clean_shutdown_interrupts_partial_upload(self):
        conn, stream = self.connect()
        conn.sendall(b"deploy 4096 00000000\n")
        self.assertEqual(frame(stream), (b"ok", b""))
        conn.sendall(b"incomplete")
        self.stop_server()
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), self.original
        )
        self.assertFalse(list(self.sd.glob("*.part")))

    def test_clean_shutdown_interrupts_log_subscription(self):
        conn, stream = self.connect()
        conn.sendall(b"log\n")
        self.assertEqual(frame(stream), (b"ok", b""))
        self.stop_server()

    def test_client_downloads_reports_without_sd_access(self):
        output = self.base / "download"
        self.cli("reports", "--output", output)
        self.assertEqual(
            (output / "reports" / "dispatch.txt").read_bytes(), b"dispatch=608214\n"
        )
        self.assertEqual(
            (output / "captures" / "frame.png").read_bytes(),
            (self.sd / "captures" / "frame.png").read_bytes(),
        )

    def test_client_deploys_verifies_and_backs_up_older_candidates(self):
        old = b"older version" * 23
        (self.sd / "WiiCompiled-Switch-old.nro").write_bytes(old)
        (self.sd / "foreign-app.nro").write_bytes(b"another application")
        candidate = self.base / "built.nro"
        candidate.write_bytes(nro(b"new built NRO" * 1024))
        backups = self.base / "backups"
        self.cli("deploy", candidate, "--backup-dir", backups)
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), candidate.read_bytes()
        )
        self.assertFalse((self.sd / "WiiCompiled-Switch-old.nro").exists())
        self.assertEqual(
            (self.sd / "foreign-app.nro").read_bytes(), b"another application"
        )
        backup_data = [path.read_bytes() for path in backups.rglob("*.nro")]
        self.assertIn(old, backup_data)
        self.assertIn(self.original, backup_data)
        self.assertFalse((self.sd / "WiiCompiled-Switch.nro.rollback").exists())

    def test_client_stops_cleanup_if_sd_readback_does_not_match(self):
        old_path = self.sd / "WiiCompiled-Switch-old.nro"
        old_path.write_bytes(b"older version")
        candidate = self.base / "built.nro"
        candidate.write_bytes(nro(b"new candidate"))
        real_download = mkw_debug.Client.download

        def corrupted_readback(client, command, destination, max_size):
            result = real_download(client, command, destination, max_size)
            if command == "get-nro":
                Path(destination).write_bytes(b"corrupt readback")
            return result

        with (
            mkw_debug.Client("127.0.0.1", self.port, 3) as client,
            patch.object(mkw_debug.Client, "download", corrupted_readback),
            self.assertRaisesRegex(mkw_debug.DebugError, "readback.*mismatch"),
        ):
            mkw_debug.deploy(client, candidate, self.base / "backups")
        self.assertTrue(old_path.exists())
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro.rollback").read_bytes(), self.original
        )

    def test_client_keeps_rollback_if_verified_backup_is_corrupted_before_finalize(
        self,
    ):
        candidate = self.base / "built.nro"
        candidate.write_bytes(nro(b"new candidate"))
        backups = self.base / "backups"
        real_download = mkw_debug.Client.download

        def corrupt_saved_backup(client, command, destination, max_size):
            result = real_download(client, command, destination, max_size)
            if command == "get-nro":
                (backups / "WiiCompiled-Switch.nro").write_bytes(b"corrupted backup")
            return result

        with (
            mkw_debug.Client("127.0.0.1", self.port, 3) as client,
            patch.object(mkw_debug.Client, "download", corrupt_saved_backup),
            self.assertRaisesRegex(mkw_debug.DebugError, "Backup.*mismatch"),
        ):
            mkw_debug.deploy(client, candidate, backups)
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro").read_bytes(), candidate.read_bytes()
        )
        self.assertEqual(
            (self.sd / "WiiCompiled-Switch.nro.rollback").read_bytes(), self.original
        )

    def test_unsafe_old_inventory_blocks_deployment_instead_of_hiding_candidate(self):
        candidate = self.base / "built.nro"
        candidate.write_bytes(nro(b"new candidate"))
        old_path = self.sd / "WiiCompiled-Switch-unsafe.nro"
        for scenario in ("oversized", "symlink"):
            with self.subTest(scenario=scenario):
                if scenario == "oversized":
                    with old_path.open("wb") as output:
                        output.truncate(mkw_debug.MAX_NRO + 1)
                else:
                    old_path.symlink_to(self.sd / "WiiCompiled-Switch.nro")
                self.assertIn(old_path.name.encode(), self.request("list-nros")[1])
                result = self.cli(
                    "deploy",
                    candidate,
                    "--backup-dir",
                    self.base / scenario,
                    check=False,
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(
                    (self.sd / "WiiCompiled-Switch.nro").read_bytes(), self.original
                )
                self.assertTrue(old_path.exists())
                old_path.unlink()


class DebugClientMalformedResponses(unittest.TestCase):
    """Small hostile peers verify the client cannot trust sizes or partial payloads."""

    def peer(self, response):
        listener = socket.socket()
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        port = listener.getsockname()[1]

        def serve():
            with listener:
                conn, _ = listener.accept()
                with conn:
                    conn.recv(4096)
                    try:
                        conn.sendall(response)
                    except (ConnectionResetError, BrokenPipeError):
                        pass

        thread = threading.Thread(target=serve, daemon=True)
        thread.start()
        self.addCleanup(thread.join, 3)
        return mkw_debug.Client("127.0.0.1", port, 1)

    def test_rejects_malformed_negative_huge_and_truncated_frames(self):
        for response in (
            b"ok -1\n",
            b"ok 9999999999\n",
            b"maybe 0\n",
            b"ok 1 extra\n",
            b"ok 4\nabc",
            b"ok " + b"1" * 130,
        ):
            with (
                self.subTest(response=response),
                self.peer(response) as client,
                self.assertRaises(mkw_debug.DebugError),
            ):
                client.request("ping")

    def test_partial_download_preserves_existing_output_and_removes_part(self):
        with tempfile.TemporaryDirectory() as temp:
            destination = Path(temp) / "report.txt"
            destination.write_bytes(b"previous complete report")
            with (
                self.peer(b"ok 20\nincomplete") as client,
                self.assertRaises(mkw_debug.DebugError),
            ):
                client.download("get reports/report.txt", destination)
            self.assertEqual(destination.read_bytes(), b"previous complete report")
            self.assertEqual(list(Path(temp).glob("*.part")), [])

    def test_remote_error_does_not_overwrite_output(self):
        with tempfile.TemporaryDirectory() as temp:
            destination = Path(temp) / "report.txt"
            destination.write_bytes(b"previous complete report")
            with (
                self.peer(b"err 6\ndenied") as client,
                self.assertRaisesRegex(mkw_debug.DebugError, "denied"),
            ):
                client.download("get reports/report.txt", destination)
            self.assertEqual(destination.read_bytes(), b"previous complete report")

    def test_report_inventory_cannot_write_outside_download_root(self):
        with tempfile.TemporaryDirectory() as temp:
            payload = b"../escaped.txt\n"
            with (
                self.peer(f"ok {len(payload)}\n".encode() + payload) as client,
                self.assertRaisesRegex(mkw_debug.DebugError, "Unsafe"),
            ):
                mkw_debug.fetch_reports(client, Path(temp) / "download")
            self.assertFalse((Path(temp) / "escaped.txt").exists())

    def test_reload_accepts_acknowledgement_followed_by_disconnect(self):
        acknowledgement = b"restart requested\n"
        with self.peer(b"ok 18\n" + acknowledgement) as client:
            self.assertEqual(client.reload(), acknowledgement)

    def test_reload_surfaces_sdk_error_after_initial_acknowledgement(self):
        error = b"restart failed: 0x00001234\n"
        response = b"ok 18\nrestart requested\n"
        response += f"err {len(error)}\n".encode() + error
        with (
            self.peer(response) as client,
            self.assertRaisesRegex(mkw_debug.DebugError, "restart failed: 0x00001234"),
        ):
            client.reload()

    def test_reload_rejects_missing_acknowledgement_and_incomplete_sdk_response(self):
        acknowledgement = b"ok 18\nrestart requested\n"
        for response in (
            b"",
            acknowledgement + b"err ",
            acknowledgement + b"err 4\nab",
            acknowledgement + b"ok 0\n",
        ):
            with (
                self.subTest(response=response),
                self.peer(response) as client,
                self.assertRaises(mkw_debug.DebugError),
            ):
                client.reload()


if __name__ == "__main__":
    unittest.main()
