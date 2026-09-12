# Hardware results — 2026-09-12

This document records the real Nintendo Switch evidence for the M2 translated-startup fast-track on 2026-09-12.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Scope

The goal of these runs was not to render graphics. It was to execute the real locally generated Mario Kart Wii translated startup path as far as possible, stop on the first attributable blocker, mirror the pinned WiiCompiled semantics, rebuild, and repeat.

The local build contains user-owned game-derived translated output and is therefore never uploaded to public CI or committed to the repository.

## Confirmed runtime state

Real hardware confirms that the local NRO can:

- initialize the Horizon/libnx host runtime;
- run the M2 local path without the libnx PrintConsole/NV framebuffer dependency;
- initialize Wii guest memory / GuestFlat;
- initialize generated data sections;
- enter WiiCompiled-translated PAL `__start` at `0x800060A4`;
- maintain an active guest context through `TRANSLATED_EXEC_ENTER`;
- execute through multiple Wii SDK/native runtime boundaries;
- execute a mixed native-HLE → translated call path (`IPCCltInit` → `IPCInit`);
- emit durable unsupported-dispatch and host-exception diagnostics to SD.

The screen remains black because the current GX FIFO bridge is deliberately a sink. No rendered Mario Kart Wii frame has been proven.

PAL `main()` at `0x8000B6B0` has **not** yet been proven reached.

## Hardware blocker sequence

### 1. `OSReport` — `0x801A25D0`

Observed as an unsupported `DIRECT` dispatch during `TRANSLATED_EXEC_ENTER`.

Pinned WiiCompiled behavior: host-side report formatting/logging only, with no guest-visible CPU or memory mutation.

Switch fix: sink the host logging side effect while preserving guest state.

Result after fix: hardware advanced beyond this boundary.

### 2. `OSGetConsoleType` — `0x8019F33C`

Observed as the next unsupported `DIRECT` boundary.

Pinned WiiCompiled behavior:

- read guest physical MEM2 size at `0x80003118`;
- return `0x00000012` for retail 64 MiB MEM2;
- otherwise return `0x10000012` for the NDEV/expanded-memory path.

Switch fix: reproduce the same guest-memory read and return value in `r3`.

Result after fix: hardware advanced beyond this boundary.

### 3. `OSGetResetCode` — `0x801A8A50`

Captured blocker:

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : DIRECT
target                : 0x801a8a50
guest pc              : 0x800060a4
r1                    : 0x80399158
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
action                : abort after durable blocker record
```

Pinned WiiCompiled behavior: avoid the Wii reset MMIO register and always return `0` (`Cold Boot`).

Switch fix: publish `r3 = 0` through the native/HLE trait.

PR: #76

Result after fix: hardware advanced beyond this boundary.

### 4. `DCZeroRange` — `0x801A16E4`

The first hardware observation was an unsupported `DIRECT` dispatch at `0x801A16E4` during `TRANSLATED_EXEC_ENTER`.

Pinned WiiCompiled behavior:

- align the guest start address down to a 32-byte cache line;
- round the covered length up to whole cache lines;
- zero the resulting guest range;
- notify the GX guest-RAM DMA-write seam;
- treat an invalid guest range as best-effort failure rather than fatal host behavior.

The first Switch HLE mirrored the valid-range zeroing but assumed invalid guest memory would throw like upstream WiiCompiled.

A later hardware run exposed the contract mismatch with this signature:

```text
fast-track stage      : TRANSLATED_EXEC_ENTER
fault address (FAR)   : 0x0000000000000000
ESR                   : 0x92000045
x0                    : 0x0000000000000000
x2                    : 0x0000000000000020
x4                    : 0x0000000000000020
x5                    : 0x00000000ffffffe0
guest context active  : YES
guest context source  : TLS
guest pc              : 0x800060a4
guest r3              : 0xffffffff
guest flat base       : nonzero
FAR in guest window   : NO
```

Interpretation:

1. guest `r3 = 0xFFFFFFFF` aligned down to `0xFFFFFFE0`;
2. the normalized size became one 32-byte cache line (`0x20`);
3. the Switch `Memory::GetPointer` slice returned `nullptr` for the invalid guest range;
4. the old HLE entered `memset(nullptr, 0, 0x20)`;
5. Horizon raised the AArch64 Data Abort at FAR `0x0`.

The important difference is that pinned WiiCompiled's full memory runtime reports invalid accesses through `Memory::AccessViolation`, while the smaller Switch slice returns `nullptr`.

Switch fix: explicitly test the returned guest pointer before `memset`. Valid-range zeroing and the GX/DMA notification remain unchanged; invalid ranges are skipped.

PR: #82

CI result before merge: 5/5 workflows green, including a Nintendo-data-free synthetic case reproducing `address = 0xFFFFFFFF`, `length = 1`.

Result after fix: the next hardware run advanced beyond `DCZeroRange` and produced a new explicit blocker instead of crashing.

### 5. `IPCCltInit` — `0x80193478`

Captured next blocker:

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : DIRECT
target                : 0x80193478
guest pc              : 0x800060a4
r1                    : 0x80399168
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
action                : abort after durable blocker record
```

Pinned WiiCompiled maps `0x80193478` to `IPCCltInit`.

Its HLE is not a simple success stub. It:

1. calls translated `IPCInit` at `0x80192F7C` so the IPC buffer globals are initialized;
2. reads the r13-relative IPC buffer-low global;
3. advances that pointer by `0x1000` for the IOS heap;
4. skips the Wii-specific interrupt handler/MMIO portion;
5. returns success.

Skipping the translated `IPCInit` call would leave the IPC arena globals unset and later filesystem/ISFS initialization would fail.

Switch fix: reproduce the same mixed native-HLE → translated call sequence and `0x1000` guest-memory adjustment.

PR: #83

CI result before merge: 5/5 workflows green, including the native→translated dispatch/link seam.

Current status: this is the latest hardware-captured guest blocker fixed in `main`. A post-#83 hardware run is required to identify the next boundary or prove PAL `main()`.

## Diagnostic-path issue discovered during hardware testing

One non-crashing black-screen run produced no `.txt` files even though the NRO itself was valid. The fast-track diagnostic path assumed `sdmc:/switch/WiiCompiled-Switch/` already existed and silently ignored file-open failure.

PR #75 changed the local fast-track to:

- create the application diagnostic directory before entering translated startup;
- keep the normal progress path under `/switch/WiiCompiled-Switch/`;
- fall back to `/switch/fast-track-progress.txt` if needed.

After that change, hardware successfully produced durable blocker records.

## Pre-guest host crash during platform initialization

A later run crashed before guest execution with this signature:

```text
fast-track stage      : MAIN_PLATFORM_INIT
fault address (FAR)   : host address
ESR                   : 0x92000047
x2                    : 0x0000000000800000
guest context active  : NO
guest pc              : 0x00000000
guest flat base       : 0x0000000000000000
FAR in guest window   : NO
```

This was not a translated PPC blocker. The guest had not started, GuestFlat was not initialized, and the faulting host operation covered exactly `0x800000` bytes (8 MiB).

That register pattern strongly matched the libnx default PrintConsole/NV initialization path:

1. `consoleInit()` selects the software framebuffer renderer;
2. the renderer creates a libnx framebuffer;
3. NV initialization uses an 8 MiB transfer-memory allocation by default;
4. `tmemCreate()` clears that allocation before creating the transfer-memory handle.

The M2 local fast-track does not need a text console, framebuffer or NV service because graphics are not yet being rendered and actionable diagnostics are persisted to SD.

The fast-track was therefore changed to start **headless**:

- no `consoleInit()` in `MKW_LOCAL_FAST_TRACK` builds;
- no fast-track `printf`/`consoleUpdate` dependency;
- public/default Nintendo-data-free builds keep the existing PrintConsole path;
- platform initialization exposes finer exception stages such as `PLATFORM_SERVICES_INIT`, `PLATFORM_ROMFS_INIT` and `PLATFORM_READY`.

Later hardware evidence validated this change directly: the process reached `TRANSLATED_EXEC_ENTER` with an active TLS guest context and nonzero GuestFlat base. The headless path is therefore confirmed in the actual local hardware fast-track, not only in CI.

## Local build provenance / verification issue

After the headless change, an incremental local build correctly produced a valid ELF/NRO but the final helper verification falsely failed.

The old check used a pipeline equivalent to:

```sh
strings "$ELF" | grep -Fq 'PLATFORM_CONSOLE_SKIPPED_FAST_TRACK'
```

with `set -o pipefail` enabled. Because `grep -q` exits as soon as it finds the marker, `strings` can receive SIGPIPE, making the overall pipeline nonzero even though the marker is present.

The helper now performs a direct binary-safe grep on the ELF instead of using the SIGPIPE-prone pipeline.

The local incremental helper also tracks the source HEAD, invalidates stale objects when the commit changes, verifies the headless marker in the ELF, and prints the NRO SHA-256 so the exact hardware-test artifact can be identified.

This tooling issue did not invalidate the already-built NRO; it only produced a false-negative post-build verification result.

## Current diagnostic files

The local fast-track may create:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

`fast-track-main-reached.txt` is the explicit hardware proof marker for PAL `main` at `0x8000B6B0`.

## Current interpretation

The 2026-09-12 evidence proves real translated startup progress well beyond the original runtime bootstrap and metadata-only product boundary.

Specifically, hardware has now proven that:

- the headless local platform path reaches real guest execution;
- PAL translated `__start` continues across multiple pinned WiiCompiled native/HLE boundaries;
- host-vs-guest faults can be distinguished through durable exception context;
- invalid guest-memory behavior at HLE seams must match the Switch memory slice contract as well as upstream intent;
- native HLE may legitimately call translated guest code and must preserve that dependency.

It does **not** prove:

- entry into PAL `main()`;
- game/resource initialization completion;
- a working GX renderer;
- a rendered frame;
- playable input/audio/gameplay.

The next real-hardware run should be classified as one of:

1. a new `fast-track-dispatch-blocker.txt` — map/fix the next pinned runtime boundary;
2. a `fast-track-exception.txt` — use stage + FAR/registers + guest context to attribute the failure;
3. a heartbeat with no blocker — investigate a non-crashing loop/stall;
4. `fast-track-main-reached.txt` — declare the PAL `main()` milestone reached and move to the first post-`main` blocker.

## Build command used for the local fast-track

```sh
git checkout main
git pull
MKW_JOBS=8 bash scripts/build-local-fast-track-incremental.sh
```

No game-derived NRO/ELF, generated C++, DOL/REL, disc image, keys, firmware or copyrighted game assets belong in this repository or public CI.
