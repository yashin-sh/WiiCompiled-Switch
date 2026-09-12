# Hardware results — 2026-09-12

This document records the real Nintendo Switch evidence for the M2 translated-startup fast-track on 2026-09-12.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Scope

The goal of these runs was not to render graphics. It was to execute the real locally generated Mario Kart Wii translated startup path as far as possible, stop on the first attributable blocker, mirror the pinned WiiCompiled semantics, rebuild, and repeat.

The local build contains user-owned game-derived translated output and is therefore never uploaded to public CI or committed to the repository.

## Confirmed runtime state

Real hardware confirms that the local NRO can:

- initialize the Horizon/libnx host runtime;
- initialize Wii guest memory / GuestFlat;
- initialize generated data sections;
- enter WiiCompiled-translated PAL `__start` at `0x800060A4`;
- execute through multiple Wii SDK/native runtime boundaries;
- remain responsive to HOME without freezing Horizon when the translated path stalls or continues on a black screen;
- emit durable unsupported-dispatch and host-exception diagnostics to SD.

The screen remains black because the current GX FIFO bridge is deliberately a sink. No rendered Mario Kart Wii frame has been proven.

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

Merge commit: `c98cf53d9e6d953b51cd20f8f012cf0bb3118ab4`

CI result before merge: 5/5 workflows green.

## Diagnostic-path issue discovered during hardware testing

One non-crashing black-screen run produced no `.txt` files even though the NRO itself was valid. The fast-track diagnostic path assumed `sdmc:/switch/WiiCompiled-Switch/` already existed and silently ignored file-open failure.

PR #75 changed the local fast-track to:

- create the application diagnostic directory before entering translated startup;
- keep the normal progress path under `/switch/WiiCompiled-Switch/`;
- fall back to `/switch/fast-track-progress.txt` if needed.

After that change, hardware successfully produced the `OSGetResetCode` blocker record above.

## Pre-guest host crash during platform initialization

A later run, after the `OSGetResetCode` HLE landed, crashed before guest execution with this signature:

```text
fast-track stage      : MAIN_PLATFORM_INIT
fault address (FAR)   : 0x0000001442583000
ESR                   : 0x92000047
x0                    : 0x0000001442583000
x2                    : 0x0000000000800000
x4                    : 0x0000001442d83000
guest context active  : NO
guest pc              : 0x00000000
guest flat base       : 0x0000000000000000
FAR in guest window   : NO
```

This is not a translated PPC blocker. The guest had not started, GuestFlat was not initialized, and the faulting host operation covered exactly `0x800000` bytes (8 MiB).

That register pattern strongly matches the libnx default PrintConsole/NV initialization path:

1. `consoleInit()` selects the software framebuffer renderer;
2. the renderer creates a libnx framebuffer;
3. NV initialization uses an 8 MiB transfer-memory allocation by default;
4. `tmemCreate()` clears that allocation with `memset(..., 0, 0x800000)` before creating the transfer-memory handle.

The M2 local fast-track does not need a text console, framebuffer or NV service because graphics are not yet being rendered and all actionable diagnostics are persisted to SD.

The fast-track is therefore changed to start **headless**:

- no `consoleInit()` in `MKW_LOCAL_FAST_TRACK` builds;
- no fast-track `printf`/`consoleUpdate` dependency;
- public/default Nintendo-data-free builds keep the existing PrintConsole path;
- platform initialization now exposes finer exception stages:
  - `PLATFORM_CONSOLE_SKIPPED_FAST_TRACK`;
  - `PLATFORM_SERVICES_INIT`;
  - `PLATFORM_SERVICES_READY`;
  - `PLATFORM_ROMFS_INIT`;
  - `PLATFORM_READY`.

This removes an unrelated pre-guest graphics/NV dependency from the blocker-driven translated-startup path while preserving it for public bootstrap/probe builds.

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

It also proves that host-only bootstrap infrastructure can still introduce failures independently of guest execution. Those paths should be removed from the local fast-track when they are not required to reach `main()`.

It does **not** prove:

- entry into PAL `main()`;
- game/resource initialization completion;
- a working GX renderer;
- a rendered frame;
- playable input/audio/gameplay.

The next real-hardware run from the headless fast-track should be classified as one of:

1. a new `fast-track-dispatch-blocker.txt` — map/fix the next pinned runtime boundary;
2. a `fast-track-exception.txt` — use the finer `PLATFORM_*`/runtime stage plus FAR/registers to attribute the failure;
3. a heartbeat with no blocker — investigate a non-crashing loop/stall;
4. `fast-track-main-reached.txt` — declare the PAL `main()` milestone reached and move to the first post-`main` blocker.

## Build command used for the local fast-track

```sh
git checkout main
git pull
MKW_JOBS=8 bash scripts/build-local-fast-track-incremental.sh
```

No game-derived NRO/ELF, generated C++, DOL/REL, disc image, keys, firmware or copyrighted game assets belong in this repository or public CI.
