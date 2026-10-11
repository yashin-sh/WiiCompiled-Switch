#!/usr/bin/env python3
"""LAN debug client for WiiCompiled-Switch (Python standard library only)."""

import argparse
import datetime
import hashlib
import io
import json
import math
import os
import re
import socket
import sys
import tempfile
import time
import zlib
from pathlib import Path, PurePosixPath

CHUNK_SIZE = 64 * 1024
MAX_NRO = 512 * 1024 * 1024
MAX_REPORT = 64 * 1024 * 1024
MAX_REPLY = 1024 * 1024


class DebugError(Exception):
    """Remote rejection, invalid protocol, or failed integrity check."""


class DebugDisconnected(DebugError):
    """A connection ended while a framed response was incomplete."""


class Client:
    def __init__(self, host, port=6543, timeout=30):
        self.host, self.port, self.timeout = host, port, timeout
        self.socket = socket.create_connection((host, port), timeout=timeout)
        self.socket.settimeout(timeout)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.socket.close()

    def send_command(self, command):
        if (
            any(ord(character) < 32 or ord(character) == 127 for character in command)
            or len(command.encode()) > 511
        ):
            raise DebugError("Invalid command")
        self.socket.sendall(command.encode("utf-8") + b"\n")

    def _header(self, idle=False, allow_eof=False):
        header = bytearray()
        while len(header) < 128:
            try:
                byte = self.socket.recv(1)
            except socket.timeout:  # noqa: UP041 -- Python 3.8 socket.timeout is distinct.
                if idle:
                    continue
                raise
            if not byte:
                if allow_eof and not header:
                    return None
                raise DebugDisconnected(
                    "Server disconnected before completing a response"
                )
            if byte == b"\n":
                match = re.fullmatch(rb"(ok|err) ([0-9]{1,10})", header)
                if not match:
                    raise DebugError("Malformed response header")
                return match[1] == b"ok", int(match[2])
            header.extend(byte)
        raise DebugError("Response header too long")

    def receive_to(self, output, max_size=MAX_REPLY, idle=False, allow_eof=False):
        header = self._header(idle=idle, allow_eof=allow_eof)
        if header is None:
            return None
        ok, size = header
        if size > (max_size if ok else MAX_REPLY):
            raise DebugError("Response exceeds allowed size")
        digest = hashlib.sha256()
        crc = 0
        error = bytearray()
        remaining = size
        while remaining:
            chunk = self.socket.recv(min(CHUNK_SIZE, remaining))
            if not chunk:
                raise DebugDisconnected("Server disconnected during response payload")
            remaining -= len(chunk)
            if ok:
                output.write(chunk)
                digest.update(chunk)
                crc = zlib.crc32(chunk, crc)
            else:
                error.extend(chunk)
        if not ok:
            raise DebugError(error.decode("utf-8", errors="replace").strip())
        return {
            "size": size,
            "sha256": digest.hexdigest(),
            "crc32": f"{crc & 0xFFFFFFFF:08x}",
        }

    def request(self, command, max_size=MAX_REPLY):
        self.send_command(command)
        output = io.BytesIO()
        self.receive_to(output, max_size)
        return output.getvalue()

    def reload(self):
        acknowledgement = self.request("reload")
        if not acknowledgement.startswith(b"restart requested\n") or fields(
            acknowledgement
        ).get("method", "hbloader") not in ("hbloader", "program"):
            raise DebugError("Invalid restart acknowledgement")
        # Acknowledgement means queued for owner preparation, not a new launch.
        # Older peers may still send a trailing SDK error before disconnect.
        trailing = io.BytesIO()
        if self.receive_to(trailing, allow_eof=True) is not None:
            raise DebugError("Unexpected extra restart response")
        return acknowledgement

    def download(self, command, destination, max_size=MAX_REPORT):
        destination = Path(destination)
        destination.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary = tempfile.mkstemp(
            prefix=destination.name + ".", suffix=".part", dir=destination.parent
        )
        try:
            with os.fdopen(fd, "wb") as output:
                self.send_command(command)
                metadata = self.receive_to(output, max_size)
                output.flush()
                os.fsync(output.fileno())
            os.replace(temporary, destination)
            return metadata
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)


def file_metadata(path):
    digest = hashlib.sha256()
    crc = size = 0
    with Path(path).open("rb") as source:
        while chunk := source.read(CHUNK_SIZE):
            size += len(chunk)
            digest.update(chunk)
            crc = zlib.crc32(chunk, crc)
    return {
        "size": size,
        "sha256": digest.hexdigest(),
        "crc32": f"{crc & 0xFFFFFFFF:08x}",
    }


def fields(payload):
    result = {}
    for line in payload.decode("utf-8").splitlines():
        key, separator, value = line.partition("=")
        if separator:
            result[key] = value
    return result


def remote_stat(client, name):
    result = fields(client.request("stat-nro " + name))
    try:
        size = int(result["size"])
        sha = result["sha256"]
        if not 0 <= size <= MAX_NRO or not re.fullmatch(r"[0-9a-fA-F]{64}", sha):
            raise ValueError()
    except (KeyError, ValueError) as exc:
        raise DebugError("Invalid remote NRO metadata") from exc
    return {"size": size, "sha256": sha.lower()}


def nro_names(client):
    names = client.request("list-nros").decode("utf-8").splitlines()
    if len(names) != len(set(names)):
        raise DebugError("Duplicate NRO inventory entries")
    for name in names:
        if not re.fullmatch(r"[A-Za-z0-9_.-]+\.nro(?:\.rollback)?", name):
            raise DebugError("Unsafe NRO inventory entry")
    return names


def assert_integrity(actual, expected, context):
    if any(actual[key] != expected[key] for key in ("size", "sha256")):
        raise DebugError(context + ": size/SHA-256 mismatch")


def private_directory(kind):
    cache = Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache"))
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    return cache / "WiiCompiled-Switch" / "debug" / kind / stamp


def ensure_private_backup(path):
    repository = Path(__file__).resolve().parents[1]
    path = Path(path).resolve()
    if path == repository or repository in path.parents:
        raise DebugError(
            "NRO backups and deployment receipts must be stored outside the repository"
        )
    if path.exists() and any(path.iterdir()):
        raise DebugError("Deployment backup directory must be empty: " + str(path))
    path.mkdir(parents=True, exist_ok=True, mode=0o700)
    return path


def deploy(client, source, backup_dir=None, reload=False, restart_seconds=180):
    if reload and (not math.isfinite(restart_seconds) or restart_seconds <= 0):
        raise DebugError("Restart deadline must be positive and finite")
    source = Path(source)
    expected = file_metadata(source)
    if source.suffix.lower() != ".nro" or not 128 <= expected["size"] <= MAX_NRO:
        raise DebugError("Candidate must be a .nro file between 128 bytes and 512 MiB")
    with source.open("rb") as candidate:
        header = candidate.read(128)
    declared_size = int.from_bytes(header[24:28], "little")
    if header[16:20] != b"NRO0" or not 128 <= declared_size <= expected["size"]:
        raise DebugError("Candidate has an invalid NRO header")
    info = fields(client.request("info"))
    if reload and (
        info.get("reload_supported") != "1"
        or info.get("reload_method") != "hbloader"
        or not re.fullmatch(r"[0-9a-f]{16}", info.get("runtime_session", ""))
        or not re.fullmatch(r"[0-9a-f]{64}", info.get("runtime_nro_sha256", ""))
    ):
        raise DebugError(
            "Running NRO lacks verified restart capability/identity; install this server revision and launch it once manually"
        )
    target_path = info.get("debug_nro_path", "")
    target_name = target_path.rsplit("/", 1)[-1]
    if not re.fullmatch(r"[A-Za-z0-9_.-]+\.nro", target_name):
        raise DebugError("Server did not provide a valid configured NRO path")
    rollback_name = target_name + ".rollback"
    backup_dir = ensure_private_backup(backup_dir or private_directory("deployments"))
    receipt = {
        "target": target_path,
        "candidate": expected,
        "backups": {},
        "status": "backing_up",
    }
    receipt_path = backup_dir / "receipt.json"

    def save_receipt():
        fd, temporary = tempfile.mkstemp(
            prefix="receipt.", suffix=".part", dir=backup_dir
        )
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as output:
                output.write(json.dumps(receipt, indent=2) + "\n")
                output.flush()
                os.fsync(output.fileno())
            os.replace(temporary, receipt_path)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)

    save_receipt()
    names = nro_names(client)
    for name in names:
        if name.endswith(".rollback") and name != rollback_name:
            raise DebugError("Inventory contains an unrelated rollback: " + name)
        if name not in (target_name, rollback_name) and not name.startswith(
            "WiiCompiled-Switch"
        ):
            raise DebugError("Inventory contains an unrelated application: " + name)
        recorded = remote_stat(client, name)
        destination = backup_dir / name
        if destination.exists():
            raise DebugError("Backup already exists: " + str(destination))
        downloaded = client.download("get-nro " + name, destination, MAX_NRO)
        assert_integrity(downloaded, recorded, "Remote backup " + name)
        assert_integrity(file_metadata(destination), recorded, "Local backup " + name)
        receipt["backups"][name] = recorded
        save_receipt()

    # Check the inventory again before replacing the configured candidate.
    if sorted(nro_names(client)) != sorted(names):
        raise DebugError("NRO inventory changed during backup; retry deployment")
    for name, recorded in receipt["backups"].items():
        assert_integrity(
            remote_stat(client, name),
            recorded,
            "Remote candidate changed during backup " + name,
        )

    def finalize(target_metadata, rollback_backup_name=None):
        if rollback_backup_name:
            recorded = receipt["backups"][rollback_backup_name]
            assert_integrity(
                file_metadata(backup_dir / rollback_backup_name),
                recorded,
                "Backup before rollback cleanup " + rollback_backup_name,
            )
            rollback_arguments = f"{recorded['size']} {recorded['sha256']}"
        else:
            rollback_arguments = "0 -"
        client.request(
            f"finalize-deploy {target_metadata['size']} {target_metadata['sha256']} {rollback_arguments}"
        )

    if rollback_name in names:
        if target_name not in receipt["backups"]:
            raise DebugError(
                "Pending rollback has no current target; recover it before deployment"
            )
        current = receipt["backups"][target_name]
        previous_readback = backup_dir / "previous-readback" / "current.nro"
        downloaded = client.download("get-nro", previous_readback, MAX_NRO)
        assert_integrity(
            downloaded, current, "Previous deployment complete SD readback"
        )
        assert_integrity(
            file_metadata(previous_readback),
            current,
            "Previous deployment local readback",
        )
        finalize(current, rollback_name)
        names.remove(rollback_name)
        if sorted(nro_names(client)) != sorted(names):
            raise DebugError(
                "NRO inventory changed while finalizing previous deployment"
            )
        receipt["status"] = "previous_rollback_finalized"
        save_receipt()

    receipt["status"] = "uploading"
    save_receipt()
    client.send_command(f"deploy {expected['size']} {expected['crc32']}")
    ready = io.BytesIO()
    client.receive_to(ready)
    if ready.getvalue():
        raise DebugError("Unexpected deployment readiness payload")
    uploaded_digest = hashlib.sha256()
    remaining = expected["size"]
    with source.open("rb") as upload:
        while remaining:
            chunk = upload.read(min(CHUNK_SIZE, remaining))
            if not chunk:
                raise DebugError("Local candidate became shorter during deployment")
            client.socket.sendall(chunk)
            uploaded_digest.update(chunk)
            remaining -= len(chunk)
        if upload.read(1) or uploaded_digest.hexdigest() != expected["sha256"]:
            raise DebugError("Local candidate changed during deployment")
    confirmation = io.BytesIO()
    client.receive_to(confirmation)
    result = fields(confirmation.getvalue())
    if (
        result.get("size") != str(expected["size"])
        or result.get("crc32") != expected["crc32"]
        or result.get("sha256", "").lower() != expected["sha256"]
    ):
        raise DebugError("Server upload verification disagrees with local candidate")
    readback = backup_dir / "readback" / "current.nro"
    downloaded = client.download("get-nro", readback, MAX_NRO)
    assert_integrity(downloaded, expected, "Complete SD readback")
    assert_integrity(file_metadata(readback), expected, "Local SD readback")
    receipt["status"] = "readback_verified"
    save_receipt()
    finalize(expected, target_name if target_name in receipt["backups"] else None)
    receipt["status"] = "rollback_finalized"
    save_receipt()
    # Only remove names enumerated by the server and backed up above.
    for name, recorded in receipt["backups"].items():
        if name not in (target_name, rollback_name):
            assert_integrity(
                file_metadata(backup_dir / name),
                recorded,
                "Backup before cleanup " + name,
            )
            client.request(f"remove-nro {name} {recorded['size']} {recorded['sha256']}")
    if nro_names(client) != [target_name]:
        raise DebugError("Project NRO cleanup incomplete; inspect private receipt")
    receipt["status"] = "complete"
    save_receipt()
    print(
        f"Deployed {target_path}: {expected['size']} bytes, SHA-256 {expected['sha256']}"
    )
    print(f"Verified backups and private receipt: {backup_dir}")
    if reload:
        receipt["restart"] = {
            "status": "requesting",
            "previous_session": info["runtime_session"],
            "method": info["reload_method"],
        }
        save_receipt()
        try:
            print(client.reload().decode("utf-8"), end="")
            receipt["restart"]["status"] = "requested"
            save_receipt()
            receipt["restart"].update(
                wait_for_runtime(
                    client,
                    info["runtime_session"],
                    target_path,
                    expected,
                    restart_seconds,
                )
            )
            save_receipt()
            print(
                "New candidate runtime and translated execution verified; gameplay and pixels remain separate"
            )
        except (DebugError, OSError, UnicodeError, ValueError) as exc:
            receipt["restart"]["status"] = "unverified"
            receipt["restart"]["error"] = str(exc)
            save_receipt()
            raise


def wait_for_runtime(client, previous_session, target_path, expected, seconds=180):
    """Require a new executed-file identity and guest milestone, never SD alone."""
    if not math.isfinite(seconds) or seconds <= 0:
        raise DebugError("Restart deadline must be positive and finite")
    deadline = time.monotonic() + seconds
    last = "server has not returned"
    while time.monotonic() < deadline:
        try:
            with Client(
                client.host,
                client.port,
                min(client.timeout, max(0.001, deadline - time.monotonic())),
            ) as probe:
                info = fields(probe.request("info"))
                if info.get("runtime_session") == previous_session:
                    if int(info.get("reload_error", "0"), 0):
                        raise DebugError(
                            "Owner reload preparation failed: " + info["reload_error"]
                        )
                    last = "the previous runtime is still serving"
                elif not re.fullmatch(r"[0-9a-f]{16}", info.get("runtime_session", "")):
                    last = "the responding runtime has no session identity"
                else:
                    if info.get("debug_nro_path") != target_path:
                        raise DebugError(
                            "Reloaded runtime uses a different configured NRO path"
                        )
                    runtime = {
                        "size": int(info.get("runtime_nro_size", "-1")),
                        "sha256": info.get("runtime_nro_sha256", ""),
                    }
                    assert_integrity(runtime, expected, "Reloaded executed NRO")
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        break
                    probe.socket.settimeout(min(client.timeout, remaining))
                    assert_integrity(
                        remote_stat(probe, target_path.rsplit("/", 1)[-1]),
                        expected,
                        "Reloaded SD candidate",
                    )
                    if int(info.get("dispatch", "0")) > 0 and info.get("stage") not in (
                        None,
                        "",
                        "startup",
                        "busy",
                    ):
                        return {"status": "runtime_execution_observed", "info": info}
                    last = (
                        "new candidate started but guest execution is not yet observed"
                    )
        except (DebugDisconnected, OSError, UnicodeError, ValueError) as exc:
            last = str(exc)
        time.sleep(min(0.5, max(0, deadline - time.monotonic())))
    raise DebugError("Restart not verified before deadline: " + last)


def diagnostic_path(path):
    parsed = PurePosixPath(path)
    if (
        not path
        or parsed.is_absolute()
        or any(part in (".", "..") for part in path.split("/"))
        or "\\" in path
        or any(character.isspace() for character in path)
    ):
        raise DebugError("Unsafe diagnostic path: " + path)
    return parsed


def fetch_reports(client, output):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    paths = client.request("reports").decode("utf-8").splitlines()
    for name in paths:
        relative = diagnostic_path(name)
        destination = (output / relative).resolve()
        if output not in destination.parents:
            raise DebugError("Diagnostic destination escapes output directory")
        metadata = client.download("get " + name, destination)
        print(f"{name}: {metadata['size']} bytes")
    print(f"Fetched {len(paths)} report(s) into {output}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--host",
        default=os.environ.get("MKW_DEBUG_HOST"),
        help="Switch LAN address (or MKW_DEBUG_HOST)",
    )
    parser.add_argument("--port", type=int, default=6543)
    parser.add_argument(
        "--timeout",
        type=float,
        default=30,
        help="Per-operation timeout in seconds; log waits through idle periods",
    )
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("ping", "info", "log", "quit", "reload"):
        commands.add_parser(name)
    reports = commands.add_parser(
        "reports", help="Fetch all permitted diagnostic files"
    )
    reports.add_argument("--output", type=Path)
    get = commands.add_parser("get", help="Fetch one permitted diagnostic file")
    get.add_argument("path")
    get.add_argument("--output", type=Path)
    upload = commands.add_parser(
        "deploy", help="Back up, deploy, read back, and clean project candidates"
    )
    upload.add_argument("nro", type=Path)
    upload.add_argument("--backup-dir", type=Path)
    upload.add_argument(
        "--reload",
        action="store_true",
        help="Restart after verified cleanup and wait for the new candidate runtime",
    )
    upload.add_argument(
        "--restart-seconds",
        type=float,
        default=180,
        help="Bounded new-runtime verification deadline",
    )
    args = parser.parse_args(argv)
    if not args.host:
        parser.error("provide --host or set MKW_DEBUG_HOST")
    if (
        not math.isfinite(args.timeout)
        or args.timeout <= 0
        or not 1 <= args.port <= 65535
    ):
        parser.error("timeout must be positive and port must be between 1 and 65535")
    try:
        with Client(args.host, args.port, args.timeout) as client:
            if args.command == "deploy":
                deploy(
                    client, args.nro, args.backup_dir, args.reload, args.restart_seconds
                )
            elif args.command == "reports":
                fetch_reports(client, args.output or private_directory("reports"))
            elif args.command == "get":
                relative = diagnostic_path(args.path)
                destination = args.output or private_directory("reports") / relative
                client.download("get " + args.path, destination)
                print(destination)
            elif args.command == "log":
                if client.request("log"):
                    raise DebugError("Unexpected log readiness payload")
                while True:
                    client.receive_to(sys.stdout.buffer, idle=True)
                    sys.stdout.buffer.flush()
            elif args.command == "reload":
                sys.stdout.buffer.write(client.reload())
                sys.stdout.buffer.flush()
            else:
                sys.stdout.buffer.write(client.request(args.command))
                sys.stdout.buffer.flush()
    except KeyboardInterrupt:
        return 130
    except (DebugError, OSError, UnicodeError, ValueError) as exc:
        print(f"mkw_debug: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
