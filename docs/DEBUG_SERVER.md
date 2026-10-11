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
debug_reload_program=0
```

The configured NRO path is a fixed deployment destination. Set it to the project
NRO to launch through hbloader. The defaults above apply when only
`debug_server=1` is specified. Launch the enabled build once manually, then
connect from a computer on the same LAN. The server binds TCP port 6543 on all
interfaces unless `debug_port` overrides it. It has no authentication or TLS;
enable it only on a trusted LAN and do not expose it to the Internet.

Program restart is unsupported in this revision: `debug_reload_program=1`
does not enable `appletRestartProgram()`. Keep that reserved option at 0,
including under hbmenu title override. Application mode cannot prove a forwarder
mapping. When hbloader supports next-load and the runtime has enabled its
coordinated owner-exit path, `reload` instead selects hbloader with the configured
NRO as both path and argv. It never restarts the retail host program.

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
`frontier`, `uptime`, `fps`, `stage`, and `debug_nro_path`. Reload status includes
`reload_supported`, `reload_method` (`hbloader` or `unavailable`),
`loader_next_load`, `reload_owner_available`, `reload_requested`, `reload_prepared`
and `reload_error`. Loader capability alone does not establish owner-exit support
or successful hardware relaunch. `runtime_session`, `runtime_nro_size` and
`runtime_nro_sha256` identify this process's launch file, captured once after SD
initialization from the actual `argv[0]`; they stay fixed through deployment and
server reinitialization. Missing launch-path evidence leaves the runtime hash
empty and blocks autonomous client qualification.
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
python3 tools/mkw_debug.py deploy WiiCompiled-Switch-local-rendered-fast-track.nro
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
7. If `--reload` was supplied, request owner-coordinated restart after cleanup,
   then wait up to `--restart-seconds` (default 180) for a different runtime
   session, the expected launch-file size/SHA-256, matching configured SD target,
   and an attributable translated dispatch/stage. This does not require another
   complete SD download; the primary full readback above remains mandatory.

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

A deployment without `--reload` leaves launch to the operator. A server revision
that predates coordinated reload must be upgraded and launched once manually;
the new CLI refuses autonomous qualification when the old runtime lacks its
capability and immutable launch identity.

```bash
python3 tools/mkw_debug.py deploy WiiCompiled-Switch-local-rendered-fast-track.nro --reload --restart-seconds 180
```

For hbloader, `envHasNextLoad()` reports next-load capability. Once the runtime
owner can drain guest fibers, finish renderer work and join all workers safely,
it enables the separate owner-readiness gate. The network worker acknowledges
`reload` and publishes a pending method; it does not invoke restart APIs or exit.
After quiescence the owner calls `envSetNextLoad(debug_nro_path, debug_nro_path)`,
then checks the debug worker's SDK join and close results before returning
normally to the loader. A failed join leaves the worker and its resources owned;
a failed close retains the completed-join state so only close is retried. Neither
failure permits a loader return or socket-service cleanup. A thread whose start
failed is closed without waiting on its unstarted handle. A successful debug
stop covers this worker; the owner also checks the separate watchdog, renderer
and guest shutdown. Paths are limited
to 384 bytes, contain no whitespace, and fit the loader ABI's 512-byte path and
2048-byte argv buffers, including terminators. This routes the configured NRO
through hbloader without restarting the retail host program.

The fast-track execution provider uses execution ABI version 2 to distinguish
cooperative cancellation from a guest failure. Dispatch hooks check a pending
reload before polling VI. A paused unsupported-dispatch diagnostic can throw
`mkw::execution::ExecutionCancelled` on the guest owner thread. Native bridges
and callback wrappers permit this exception and rethrow it past fault handlers;
CPU, FPR, mutex and callback-depth guards therefore run during stack unwinding.
Each started guest fiber owns stable CPU storage, catches the stop completely,
and resumes its caller with the saved host TLS/FP state. The scheduler drains
remaining started fibers without entering unstarted guest functions, then
destroys only quiescent contexts. It retains guest backing if draining fails.

After successful next-load preparation, main checks both diagnostic-worker
joins before renderer cleanup. Renderer shutdown releases Aurora caches and
Switch copy/present globals and explicitly destroys the Dawn device before
dropping its references. Guest memory is released last. A failed shutdown keeps
the owner alive; only a complete shutdown allows normal return from main.
The host contracts exercise native refusal unwinding, nested and cyclic fiber
handoffs, callback-local CPU lifetime, reporter pauses, worker-join retries and
the real main function's normal return. They use simulated platform/loader
services; their renderer is a seam and does not measure Dawn worker counts.
Some pinned Aurora static GPU bindings release their final references in
`fini_array`. The audited devkit runtime initializes that array's constructors
but does not register its destructors with newlib exit. The Switch
`userAppExit` hook explicitly dispatches `__libc_fini_array` once, after main
and newlib exit procedures, before libnx service teardown and loader return.
Device destruction drains pending work, while destruction of its worker pool
joins its threads; renderer handle release alone does not prove that the pool
has already been destroyed. The private rendered gate checks the final linked
exit hook, destructor array and loader path in addition to compilation.
Autonomous console relaunch still requires hardware validation.

If next-load is unavailable, `reload` returns an error. Program restart remains
unsupported even with `debug_reload_program=1`, an application context or newer
system version. A hbmenu title-override trial on 2026-10-11 restarted the retail
host game instead of this NRO; application mode is insufficient. Any future
forwarder program-restart path needs a verified mapping and a separately validated
final shutdown order before it can be offered.

The server
rejects a missing or invalid NRO, unfinished `.part`/`.rollback` deployment,
duplicate requests, and candidate mutations while reload is pending. A failed
acknowledgment never publishes a request. Owner preparation failure clears the
pending request, leaves the NRO/server alive, logs the error and exposes its code
through `info`; it must not lead to normal exit. The acknowledgment means queued,
not relaunched. Standalone `reload` only acknowledges the request; `deploy
--reload` additionally verifies the new candidate runtime and guest milestone,
and keeps a private restart result in the deployment receipt. A blocked frontier
can prove execution without proving gameplay or a menu image.

These protocol and SDK-seam contracts do not establish a successful coordinated
hardware relaunch. The owner-readiness gate defaults to false and must only be
enabled by the implemented and tested runtime exit path. The new revision still
needs its exact build, deployment and attributable automatic-relaunch trial.
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
| `reload` | `restart requested` plus `method=hbloader`, then connection close; unsupported/unready runtimes receive only an error. Later owner failures appear in `info` and logs |
| `quit` | Acknowledgment then connection close |

CRC-32 is eight hexadecimal digits; SHA-256 is 64 hexadecimal digits. Deployment
metadata contains `path`, `size`, `crc32`, and `sha256`. The Python client caps
ordinary responses at 1 MiB, diagnostic downloads at 64 MiB, and NRO transfers at
512 MiB, with bounded header reads and streaming file hashing.

## Console installation on 2026-10-11

The rendered candidate built from `10dc30d` is installed as
`sdmc:/switch/WiiCompiled-Switch-debug-10dc30d.nro`. Its 73,711,672 bytes and
SHA-256 `79b709bda353e1e7630d4c62dd72109ea48ce4c212b1a79f28b4dadfa99fdb14`
pass complete USB SD readback. The SD configuration is independently reread with
`debug_server=1`, port 6543, and that exact deployment path. The older project NRO
is backed up locally, checked against its recorded size/SHA-256, and removed;
the new candidate is the only project NRO in the SD `switch` directory.

The user first-launched the candidate after exiting Haze. On the console,
`ping` returns `pong`; `info`, framed log streaming, report enumeration/download,
and `stat-nro` all succeed. The server independently reports the installed
candidate's exact size/SHA-256. A first report download retrieves 48 files;
pre-existing SD reports and captures are not automatically fresh-run evidence.

After about 40 seconds, execution reaches dispatch 655,654 and pauses at
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR@0x80170f2c`, with guest PC `0x800060A4`.
`info` reports `DEBUG_FRONTIER_PAUSED`, and the current blocker report is fetched
over TCP while the server remains accessible. This validates the hardware
server and unsupported-frontier hold, without establishing gameplay completion.
This initial installation did not validate forwarder restart or network NRO
replacement. Later verified LAN replacement and texture progression are recorded
in the [RGB5A3 candidate result](GX_RGB5A3_REPEAT_2026-10-11.md). Automatic NRO
restart remains unvalidated; the host-game restart above motivated removal of
program restart from the coordinated hbloader candidate.
Binary products, backups, and raw receipts stay private.
