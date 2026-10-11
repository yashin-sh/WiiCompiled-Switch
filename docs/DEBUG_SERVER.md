# Switch LAN debug server

The NRO includes an optional debug server independent of the guest PPC execution
and graphics bridges. It is disabled by default. The first version supports live
logs, execution status, diagnostic downloads, verified NRO deployment, and restart
requests. It does not implement controller injection or screenshot capture.

## Enable on the Switch

In `sdmc:/switch/WiiCompiled-Switch/Config/debug.ini`, add:

```ini
debug_server=1
debug_port=6543
debug_nro_path=sdmc:/switch/WiiCompiled-Switch/WiiCompiled-Switch.nro
```

The configured NRO path is a fixed deployment destination. Set it to the project
NRO that your HOME forwarder actually launches. The defaults above apply when
only `debug_server=1` is specified. Launch the enabled build once manually, then
connect from a computer on the same LAN. The server binds TCP port 6543 on all
interfaces unless `debug_port` overrides it. It has no authentication or TLS;
enable it only on a trusted LAN and do not expose it to the Internet.

With the server enabled, an unsupported guest frontier writes the usual durable
SD report and holds guest execution so the independent server can still deliver
status, reports, and deployment/restart commands. With the server disabled, the
usual abort behavior remains. A real host CPU fault cannot safely use this hold
mechanism and may terminate the application.

## PC client

The client requires Python 3.8 or newer and uses only the standard library:

```bash
export MKW_DEBUG_HOST=192.168.1.42
python3 tools/mkw_debug.py ping
python3 tools/mkw_debug.py info
python3 tools/mkw_debug.py log
python3 tools/mkw_debug.py reports
```

Alternatively, pass `--host 192.168.1.42` before the command. `--port` and
`--timeout` also precede the command. The default timeout is 30 seconds per socket
operation; `log` waits through idle periods and stops on Ctrl+C or disconnect.
Log history is bounded, so a slow or disconnected subscriber can miss older
entries. The stream mirrors the fast-track diagnostic writer, stage/dispatch
telemetry (dispatch samples at most once per second), and rendered diagnostic
reports; arbitrary stdout and upstream runtime logging are not automatically
captured. Streaming logs complements the existing SD diagnostics.

`info` returns newline-separated status fields, including `dispatch`, `guest_pc`,
`frontier`, `uptime`, `fps`, `stage`, `debug_nro_path`, and `reload_supported`.
Counters describe the latest observed execution state. FPS is the total observed
present count divided by uptime since server start; it is a lifetime average and
can remain zero before rendering begins. The server accepts at most four
simultaneous clients, and network transfer progress times out after 10 seconds
without progress.

`reports` enumerates permitted diagnostic files and downloads them recursively.
The allowlist includes root `.txt`, `.log`, `.png`, and `.mkwr` files and
the project's logs, reports, and captures directories (initial capitals are also
accepted). Listing is bounded to four directory levels and about 64 KiB of paths.
Paths are relative to `sdmc:/switch/WiiCompiled-Switch/`. It does not expose
arbitrary SD contents.

```bash
python3 tools/mkw_debug.py reports --output /tmp/mkw-reports
python3 tools/mkw_debug.py get fast-track-post-main-last-dispatch.txt
```

Downloads default to a timestamped directory under
`$XDG_CACHE_HOME/WiiCompiled-Switch/debug/` (or
`~/.cache/WiiCompiled-Switch/debug/`). Files are received into temporary `.part`
files before local rename. Keep raw reports and NRO backups private.

## Deploy and restart

For an existing local rendered build environment:

```bash
./scripts/build-local-rendered-fast-track.sh
python3 tools/mkw_debug.py deploy WiiCompiled-Switch-local-rendered-fast-track.nro --reload
```

The build helper requires the existing local game product, generated shards, and
rendered dependencies described in the project build documentation. Choose the
output NRO for your build mode when using a different helper or capture variant.

Deployment proceeds in this order:

1. Read the configured target and project NRO inventory from the server.
2. Record each old candidate's remote size/SHA-256, download it to a private
   local backup, and verify both the transfer and the saved backup. This includes
   a leftover `<target>.rollback` from an earlier interrupted deployment.
3. If an earlier rollback exists, download the complete current target from SD
   and verify its size/SHA-256. Only then finalize that earlier deployment, using
   the target hash and the verified rollback backup hash, before uploading again.
4. Upload the new NRO. The server checks the `.part` upload's NRO header and
   size/CRC-32, reads it back, renames the existing target to `<target>.rollback`,
   and renames `.part` to the now vacant target path. A commit failure restores
   the rollback when possible. The old target remains on SD until finalization.
5. Download the complete new candidate from SD and verify size/SHA-256 against
   the local build. Reverify the saved old target backup, then explicitly send
   `finalize-deploy` with both hashes. The server verifies both SD files before
   removing the rollback. An upload acknowledgment alone does not pass this gate.
6. Remove older project candidates only after verified local backups and full
   new-candidate readback. Each removal rechecks the remote file size/SHA-256.
   Verify the inventory contains only the configured candidate.
7. If `--reload` was supplied, request restart.

libnx's SD `rename` delegates directly to the Horizon file rename service, so
deployment uses explicit rotation without assuming replacement of an existing
destination. The multi-step replacement is not atomic. If startup finds the
target missing and a safe regular rollback present, it restores the rollback.
Power loss between the two renames can leave the configured target absent. A
forwarder pointing only to that missing file cannot launch the server to recover
it. Restore a verified PC backup manually, or launch a recovery copy through
hbmenu so server startup can restore the rollback. This is not an atomic
power-loss recovery guarantee.
See the [libnx filesystem implementation](https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/devices/fs_dev.c).

The inventory covers the configured target, its exact `.rollback` companion,
and `WiiCompiled-Switch*.nro` files in its directory. Other application names and
game data are preserved. Use a dedicated project directory and retain the
project's NRO naming convention for older candidates to participate in cleanup.
The client saves backups, the new
SD readback, and `receipt.json` outside the repository; `--backup-dir` can choose
another private directory outside the checkout. Failure stops the procedure
before further cleanup or restart and reports an error. The already-written
candidate and verified backups remain inspectable.

`reload` uses `appletRestartProgram()` only in an application or system-application
context on system version 5.0.0 or later. hbmenu in applet mode rejects the command;
title override can provide an application context, but support alone does not
prove which NRO the title will restart. Your HOME forwarder must already point at
`debug_nro_path`; deployment cannot change that launch mapping. The acknowledgment
means the restart was requested; it does not confirm that the replacement NRO has
started. The client then waits for connection closure or a trailing error frame,
so an SDK restart failure is surfaced to the CLI and in the log.
`quit` closes the client's connection; it does not terminate the NRO or disable
the server.

```bash
python3 tools/mkw_debug.py reload
python3 tools/mkw_debug.py quit
```

A successful transfer/restart request is not hardware gameplay or renderer
validation. Continue recording the observed dispatch frontier, screenshots from
the existing frame-dump workflow, and hardware results separately.

## Wire protocol

Commands are UTF-8 lines terminated by `\n`. Every response is an ASCII header
`ok <decimal-size>\n` or `err <decimal-size>\n`, followed by exactly that many
payload bytes. Errors carry a human-readable message. There is no additional
delimiter after the payload. Clients may send another command on the same
connection after a complete response.

| Command | Payload or behavior |
| --- | --- |
| `ping` | `pong` text |
| `info` | `key=value` status lines |
| `log` | Initial `ok 0\n`, then repeated framed log payloads |
| `reports` | Newline-separated relative file paths |
| `get <relative-path>` | Permitted diagnostic file bytes |
| `list-nros` | Newline-separated project candidate basenames, including the exact configured `.rollback` companion |
| `stat-nro [basename]` | `path`, `size`, and `sha256` metadata lines |
| `get-nro [basename]` | Project NRO bytes; defaults to configured target |
| `deploy <size> <crc32hex>` | Initial `ok 0\n`, exact upload bytes, then metadata response |
| `finalize-deploy <target-size> <target-sha256> <rollback-size> <rollback-sha256>` | Verify current target and old rollback before removing the rollback; use `0 -` when no rollback exists |
| `remove-nro <basename> <size> <sha256hex>` | Verify then delete an older project candidate; current target cannot be removed |
| `reload` | Restart request acknowledgment, then connection close; an SDK failure adds a trailing `err` frame before close. Unsupported launches receive only an error |
| `quit` | Acknowledgment then connection close |

CRC-32 is eight hexadecimal digits; SHA-256 is 64 hexadecimal digits. Deployment
metadata contains `path`, `size`, `crc32`, and `sha256`. The Python client caps
ordinary responses at 1 MiB, diagnostic downloads at 64 MiB, and NRO transfers at
512 MiB, with bounded header reads and streaming file hashing.
