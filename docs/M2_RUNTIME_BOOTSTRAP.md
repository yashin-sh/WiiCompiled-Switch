# M2 — Horizon runtime bootstrap / translated fast-track

Status: **core bootstrap and real translated startup hardware-validated on Nintendo Switch through 2026-09-12**. The current target is PAL Mario Kart Wii `main()` at `0x8000B6B0`.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Reach a stable WiiCompiled runtime on Horizon, link a locally generated translated Mario Kart Wii product, execute the real translated startup path, and iteratively remove the first concrete runtime blocker until the game reaches `main()`.

Graphics and audio completeness are not prerequisites for this milestone. The current GX FIFO bridge is intentionally a sink, so a black screen is expected during the boot fast-track.

## Validated runtime foundation

The following pieces have been validated through CI and/or real Switch hardware:

1. Horizon lifecycle and SD filesystem services;
2. Wii guest-memory initialization and GuestFlat mapping;
3. WiiCompiled `HostContext` scheduler/context-switch backend on AArch64;
4. generated data-section initialization;
5. translated execution handoff into PAL `__start` (`0x800060A4`);
6. translated direct/indirect dispatch integration;
7. host exception diagnostics with guest-context attribution;
8. durable unsupported-dispatch diagnostics;
9. early Wii SDK cache/timing/interrupt/exception HLE;
10. EXI/SI startup coverage and basic EXI transaction HLE.

The local fast-track now runs real WiiCompiled-translated Mario Kart Wii code on hardware rather than stopping at the old metadata-only translated-product boundary.

## Current translated startup path

The active path is conceptually:

```text
Horizon/libnx entry
  ↓
runtime services
  ↓
Memory::Init / GuestFlat
  ↓
HostContext worker
  ↓
generated data initialization
  ↓
translated execution handoff
  ↓
PAL __start (0x800060A4)
  ↓
early Wii SDK / OS initialization
  ↓
remaining blockers
  ↓
PAL main (0x8000B6B0)
```

`main()` has **not yet been proven on hardware**.

## Hardware-driven blocker sequence

Recent real-Switch runs have provided concrete unsupported-dispatch boundaries inside translated startup. Each fix mirrors the exact semantics of the pinned WiiCompiled runtime rather than blindly no-oping guest-visible behavior.

### `OSReport` — `0x801A25D0`

Pinned WiiCompiled implements this as host-only logging. It does not alter guest CPU state or guest memory. The Switch HLE therefore sinks the host logging side effect while preserving guest-visible state.

### `OSGetConsoleType` — `0x8019F33C`

Pinned WiiCompiled reads guest MEM2 size at `0x80003118` and returns:

- `0x00000012` for retail 64 MiB MEM2;
- `0x10000012` for the expanded/NDEV path.

The Switch HLE mirrors that behavior in guest `r3`.

### `OSGetResetCode` — `0x801A8A50`

Pinned WiiCompiled deliberately avoids real Wii reset MMIO and returns `0` (`Cold Boot`). The Switch HLE now mirrors that result in guest `r3`.

The hardware run that exposed `OSGetResetCode` reported:

```text
kind             : DIRECT
target           : 0x801a8a50
guest pc         : 0x800060a4
r1               : 0x80399158
r2               : 0x8038efa0
r3               : 0x00000000
r13              : 0x8038cc00
fast-track stage : TRANSLATED_EXEC_ENTER
```

That blocker is fixed in `main` after PR #76.

## Diagnostics

The local fast-track creates the application diagnostic directory before entering translated startup and writes under:

```text
sdmc:/switch/WiiCompiled-Switch/
```

The main files are:

- `fast-track-progress.txt` — coarse entry/return/block state for translated startup;
- `fast-track-heartbeat.txt` — periodic translated-dispatch liveness snapshot;
- `fast-track-main-reached.txt` — written when PAL `main` (`0x8000B6B0`) is dispatched;
- `fast-track-dispatch-blocker.txt` — first unsupported direct/indirect translated boundary;
- `fast-track-exception.txt` — libnx exception record with AArch64 and guest context.

If the primary directory cannot be opened, `fast-track-progress.txt` also has a fallback under `sdmc:/switch/`.

The diagnostics intentionally record only technical runtime state; no Nintendo game data or generated translated code is committed.

## Why `+` may not exit during translated boot

The current local fast-track invokes translated startup synchronously inside `runtime_bootstrap::start()`. The normal outer Horizon loop, including `appletMainLoop()` and `HidNpadButton_Plus` polling, runs only after that call returns.

Therefore `+` is not expected to terminate the process while translated startup is still executing. HOME remains available because Horizon itself is healthy; closing the title through HOME is the current hardware-test exit path for a non-crashing stall.

## Why the screen is black

The current `GX_HLE_FIFO_Write8/16/32/Float/Burst` bridge is a temporary sink. It consumes translated GX FIFO writes without presenting them to a real Switch graphics backend.

Consequences:

- a black screen does **not** imply the translated CPU path is stalled;
- reaching `main()` can be proven independently through `fast-track-main-reached.txt`;
- first-frame work belongs to M3, where the FIFO sink must be replaced by a real GX → Switch renderer/backend.

## Local build path

For the current private translated fast-track:

```sh
git checkout main
git pull
MKW_JOBS=8 bash scripts/build-local-fast-track-incremental.sh
```

The user-owned game inputs and generated translated product remain local-only. Do not commit or upload DOL/REL inputs, disc images, generated game-derived C++/objects, game-containing NRO/ELF files, keys, firmware, or extracted copyrighted assets.

## Public CI boundary

Public CI stays Nintendo-data-free. It validates the platform/runtime code and synthetic execution seams with fabricated control probes, not Mario Kart Wii data.

The local game-containing build is a separate private workflow that links the WiiCompiled-generated product into the same AArch64 NRO.

## Next slices

1. run the current post-PR-#76 NRO on hardware;
2. capture the next `fast-track-dispatch-blocker.txt`, exception, heartbeat, or `fast-track-main-reached.txt`;
3. if a blocker appears, map its PAL address against the pinned WiiCompiled runtime and preserve its actual guest semantics;
4. repeat until PAL `main` (`0x8000B6B0`) is reached;
5. then attribute the first post-`main` blocker before expanding into game subsystem bring-up;
6. begin the real GX → Switch graphics backend only when the pre-graphics runtime path is stable enough to make first-frame work meaningful.

See `HARDWARE_RESULTS_2026-09-12.md` for the current hardware evidence.
