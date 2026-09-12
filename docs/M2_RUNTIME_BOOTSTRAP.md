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
9. headless local platform initialization that bypasses the unrelated PrintConsole/NV framebuffer path;
10. early Wii SDK cache/timing/interrupt/exception HLE;
11. EXI/SI startup coverage and basic EXI transaction HLE;
12. mixed native-HLE → translated dispatch, including `IPCCltInit` → `IPCInit`;
13. SDA-backed host HLE state publication, including `__OSInitSTM`;
14. initial NAND/ISFS bootstrap linking SD-backed host storage with required guest NAND path/state.

The local fast-track now runs real WiiCompiled-translated Mario Kart Wii code on hardware rather than stopping at the old metadata-only translated-product boundary.

## Current translated startup path

The active path is conceptually:

```text
Horizon/libnx entry
  ↓
headless platform/runtime services
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
OSReport → OSGetConsoleType → OSGetResetCode
  ↓
DCZeroRange → IPCCltInit → __OSInitSTM → NANDInit
  ↓
remaining blockers
  ↓
PAL main (0x8000B6B0)
```

`main()` has **not yet been proven on hardware**.

## Hardware-driven blocker sequence

Recent real-Switch runs have provided concrete unsupported-dispatch boundaries and attributable host exceptions inside translated startup. Each fix mirrors the pinned WiiCompiled semantics rather than blindly no-oping guest-visible behavior.

### `OSReport` — `0x801A25D0`

Pinned WiiCompiled implements this as host-only logging. It does not alter guest CPU state or guest memory. The Switch HLE therefore sinks the host logging side effect while preserving guest-visible state.

### `OSGetConsoleType` — `0x8019F33C`

Pinned WiiCompiled reads guest MEM2 size at `0x80003118` and returns:

- `0x00000012` for retail 64 MiB MEM2;
- `0x10000012` for the expanded/NDEV path.

The Switch HLE mirrors that behavior in guest `r3`.

### `OSGetResetCode` — `0x801A8A50`

Pinned WiiCompiled deliberately avoids real Wii reset MMIO and returns `0` (`Cold Boot`). The Switch HLE mirrors that result in guest `r3`.

### `DCZeroRange` — `0x801A16E4`

The initial Switch port correctly mirrored the valid-range zeroing semantics but assumed an invalid guest lookup would throw like upstream WiiCompiled. The Switch `Memory::GetPointer` slice instead returns `nullptr`.

Real hardware exposed the mismatch with guest `r3 = 0xFFFFFFFF`: the address aligned down to `0xFFFFFFE0`, the range length became `0x20`, `GetPointer` returned `nullptr`, and `memset(nullptr, 0, 0x20)` caused an AArch64 Data Abort at FAR `0x0` while the guest context was active.

The HLE now checks the returned pointer before entering libc. Valid ranges are still zeroed and the GX/DMA notification seam is preserved; invalid ranges are skipped, matching the pinned best-effort behavior.

### `IPCCltInit` — `0x80193478`

Pinned WiiCompiled does **not** simply return success. Its HLE:

1. calls translated `IPCInit` at `0x80192F7C` so IPC buffer globals are initialized;
2. reads the r13-relative IPC buffer-low global;
3. advances it by `0x1000` for the IOS heap;
4. skips Wii-specific interrupt/MMIO setup;
5. returns success.

The Switch HLE mirrors that mixed native→translated sequence. Nintendo-data-free CI validates the dispatch/link seam.

### `__OSInitSTM` — `0x801AB848`

Pinned WiiCompiled avoids opening real Wii `/dev/stm/*` IOS devices on the host. Instead its HLE publishes the guest-visible STM bookkeeping that later reset logic checks through the SDA block relative to `r13`:

- `r13 - 0x62CC` = initialized flag `1`;
- `r13 - 0x62C8` = stable non-zero immediate handle `0x00535401`;
- `r13 - 0x62C4` = stable non-zero event-hook handle `0x00535402`;
- return value = success (`r3 = 1`).

The Switch HLE mirrors those values. If the SDA base/range is invalid it returns failure (`r3 = 0`) without touching unmapped guest memory. Wii STM Power/Reset callback pointers remain unset because the Switch fast-track does not generate the corresponding Wii hardware interrupt.

### `NANDInit` — `0x8019E18C`

This is the latest hardware-captured `DIRECT` blocker.

Pinned WiiCompiled's synchronous NAND bootstrap performs host and guest setup rather than merely returning success:

1. initialize its host-side ISFS/NAND support;
2. derive the current four-character game code from guest memory at `0x80000000`, with PAL `RMCP` fallback;
3. create the title data directory for `/title/00010004/<gamecode>/data`;
4. write that Wii-style path into guest `NANDHomeDir` at `0x80346D20`;
5. write initialized state `2` at `0x80386848`;
6. return `NAND_RESULT_OK`.

The Switch HLE mirrors those guest-visible semantics and maps the host directory onto the existing SD-backed `mkw::horizon_runtime_services::nand_root()`. Horizon never opens the Wii IOS `/dev/fs` device. Host directory creation is best-effort; guest address validation prevents a malformed fixed guest range from becoming a host fault.

Nintendo-data-free CI covers `InvokeDirectCpu<0x8019E18C>` through the same translated-execution seam used by the local fast-track.

## Headless platform validation

A pre-guest hardware crash was previously observed during `MAIN_PLATFORM_INIT`, with no guest context, no GuestFlat mapping, and an 8 MiB host clear matching the libnx PrintConsole/NV transfer-memory path.

The local fast-track now skips `consoleInit()` and all fast-track PrintConsole output. Subsequent hardware runs reached `TRANSLATED_EXEC_ENTER` with an active TLS guest context, confirming that the headless path is not merely compiled but actually exercised on hardware.

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

The helper invalidates stale incremental objects when Git HEAD changes, verifies the headless fast-track marker in the resulting ELF without a `pipefail`/SIGPIPE-prone pipeline, and prints the NRO SHA-256 so the tested hardware artifact can be identified exactly.

The user-owned game inputs and generated translated product remain local-only. Do not commit or upload DOL/REL inputs, disc images, generated game-derived C++/objects, game-containing NRO/ELF files, keys, firmware, or extracted copyrighted assets.

## Public CI boundary

Public CI stays Nintendo-data-free. It validates the platform/runtime code and synthetic execution seams with fabricated control probes, not Mario Kart Wii data.

The local game-containing build is a separate private workflow that links the WiiCompiled-generated product into the same AArch64 NRO.

## Next slices

1. build the current `main` local fast-track and run it on hardware;
2. capture the next `fast-track-dispatch-blocker.txt`, exception, heartbeat, or `fast-track-main-reached.txt`;
3. if a blocker appears, map its PAL address against the pinned WiiCompiled runtime and preserve its actual guest semantics;
4. repeat until PAL `main` (`0x8000B6B0`) is reached;
5. then attribute the first post-`main` blocker before expanding into game subsystem bring-up;
6. begin the real GX → Switch graphics backend only when the pre-graphics runtime path is stable enough to make first-frame work meaningful.

See `HARDWARE_RESULTS_2026-09-12.md` for the current hardware evidence.
