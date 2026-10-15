# Native Deko3D GPU submission failure — 2026-10-15

The operator reports incorrect display or another crash after the overnight
repairs. Fresh native logs and the latest Atmosphere report attribute this run
to release candidate `c2d6785`. The crash persists; neither character-selection
correctness nor a complete race is validated.

## Attribution and checks

Console argv names `WiiCompiled-Switch-native-deko3d-c2d6785.nro`. Complete SD
readback matches the recorded artifact:

```text
source:        c2d678569067ca8efe4ac0ceefe677a868dc8c91
NRO bytes:     74444944
NRO SHA-256:   3daafd3ab5f4c9de7ba5b8b139e3f4bf221ba27591af8c7d1a85d489c40ada40
ELF bytes:     983182248
ELF SHA-256:   2881107bd36b0539c8d56b18c4a32803e25fc144eca16accfcbf7f642413fdb9
```

The console transcript, root stdout/stderr and Atmosphere report each pass two
independent readbacks. The crash report is 65,248 bytes; its SHA-256 is
`d2e281716bb30f534278a7f9adf79516f30de48eab497d6e86fa8d3faa282ecb`.
Raw files and retrieval receipts remain private. The exact ELF is independently
rehashed before symbolization and retained with the executed NRO.

The new stderr again records all 21 GPU smoke tests passing, then multiple game
scenes and periodic rendering diagnostics. This does not validate menu rendering,
complete races or FPS. Process memory reporting alone does not establish an
allocation failure.

## Observed stop

Atmosphere reports a **User Break**, rather than the recursive stdio exception
seen on the first candidate. The exact ELF resolves the primary chain to:

```text
svcBreak / diagAbortWithResult
dk::detail::RaiseError(DkResult)
dk::detail::Queue::flush
dawn::native::deko3d::Device::SubmitPendingCommands
dawn::native::deko3d::Queue::SubmitImpl
aurora::encode_sealed_frame / frame_worker_main
```

The return address `+0x2d5dd18` follows the release Deko3D failure call at
`+0x2d5dd14`. Disassembly shows that its error branches share that call site.
The [queue implementation](https://github.com/devkitPro/deko3d/blob/master/source/dk_queue.cpp)
can reject a flush of an already failed queue, or abort after a failed channel
kickoff without a reported GPU error. This report does not distinguish those
paths or identify the underlying invalid GPU operation. Release
[RaiseError](https://github.com/devkitPro/deko3d/blob/master/source/dk_device.cpp)
aborts with a result code and omits the debug message/callback. The new evidence
locates the failure in GPU submission; it does not establish a final cause.

## Diagnostic candidate deployed

The existing `AURORA_SWITCH_DEKO3D_DEBUG=ON` build option links `libdeko3dd.a`.
The full private rebuild passes artifact, packaging and source/dependency
validation. The ELF exposes `RaiseError(DkResult, char const*)`, confirming the
message-bearing debug implementation, and the strong 64 KiB exception stack.
This is a diagnostic variant of the same repaired runtime, not a gameplay fix.

```text
source:        c2d678569067ca8efe4ac0ceefe677a868dc8c91
mode:          AURORA_SWITCH_DEKO3D_DEBUG=ON
NRO bytes:     74473616
NRO SHA-256:   22c6e15e69677e532a942a25849766145bad4d0458f386aced317ae3b4c1751c
SD path:       sdmc:/switch/WiiCompiled-Switch-native-deko3d-c2d6785-debug.nro
```

Deployment completes at 2026-10-15 05:38:01 UTC (07:38:01 CEST). Complete new-NRO
size/SHA-256 readback precedes old-NRO backup, recorded-metadata verification and
removal. Independent hashing and a fresh SD inventory verify that only the new
project candidate remains. Configuration and the extraction marker remain
byte-identical; other applications and game/NAND data are preserved. Cleanup
receipts remain private. The diagnostic candidate needs a new console trial to
capture the detailed Deko3D error; its performance is not measured.

## Debug trial: channel kickoff rejected without a GPU notification

The next independently read console/stderr identifies `c2d6785-debug`. The
operator sees `dkQueueFlush` and application code `2-0001`; the verified stderr
reports `gpu channel kickoff failed, but no error was reported`. This selects
the channel-kickoff failure path rather than a flush of an already failed queue.
It still omits the `Result` returned by libnx, so it does not prove an invalid
GPU method, a page fault or memory exhaustion.

The product patch now wraps `nvGpuChannelKickoff` at link time. It logs the exact
failing result, module/description and queued-entry counts, calls the real
function once and returns its unchanged result. It adds no retry or recovery.
A compiled linker-wrap contract verifies forwarding, return/state preservation,
success silence and failure-only output. Complete-file audits accept only the
original, previous audited or new repaired product sources; the reference stays
unchanged. The next private build and console trial are tracked separately.

The full native rebuild of `c82e03e-debug` passes artifact/source/dependency
validation. Disassembly confirms that the actual Deko3D `Queue::flush` calls
the wrapper and that the wrapper calls the real kickoff exactly once.

```text
source:        c82e03ef642ac6b80b0a4319cc6721e1c315d674
mode:          AURORA_SWITCH_DEKO3D_DEBUG=ON
NRO bytes:     74473616
NRO SHA-256:   cdd6ffd43b1c563fa43c2c35cb1da1da9a242445db1e6e3e51b117268add3489
SD path:       sdmc:/switch/WiiCompiled-Switch-native-deko3d-c82e03e-debug.nro
```

Deployment completes at 2026-10-15 05:51:04 UTC (07:51:04 CEST). Full new-NRO
readback, recorded old-NRO backup verification/removal and a fresh final inventory
pass. Configuration, extraction marker, other applications and game/NAND data
remain preserved; the previous diagnostic NRO and exact ELF are retained.
This adds evidence collection and needs a new trial; it does not repair the
unidentified submission failure or establish playability/FPS.

## Instrumented trial: NVIDIA kickoff timeout

The subsequent `c82e03e-debug` trial fails again. Fresh argv identifies the
instrumented NRO; independently read logs and complete SD NRO readback match the
recorded candidate. The wrapper records result **`0x00000d5c`**, module **348**,
description **6**, with **256 queued entries, 256 remaining** after failure.
In the exact [libnx 4.12.0 result definitions](https://github.com/switchbrew/libnx/blob/v4.12.0/nx/include/switch/result.h),
this is `LibnxNvidiaError_Timeout`. The
[NVIDIA error conversion](https://github.com/switchbrew/libnx/blob/v4.12.0/nx/source/services/nv.c)
maps the driver's timeout result to that description. The retained exact ELF
and private receipt remain available for attribution.

This establishes a timeout returned by channel kickoff, without a reported GPU
error notification. It does not identify the workload or driver condition that
causes the timeout. The submission failure and incorrect display remain open;
no recovery, complete race or measured FPS is claimed.
