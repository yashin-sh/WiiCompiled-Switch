# M2 — Horizon runtime bootstrap / translated fast-track

Status: **core bootstrap, PAL `main()`, and post-main translated execution are hardware-validated on Nintendo Switch through 2026-09-16**.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Maintain a stable WiiCompiled runtime on Horizon, link a locally generated translated Mario Kart Wii product, and advance real hardware execution one concrete blocker at a time until game/resource initialization is complete enough to support the first rendered frame.

The original M2 goal of reaching PAL `main()` (`0x8000B6B0`) is complete. The active work is now the post-main fast-track tracked in issue #117.

Graphics and audio completeness are not prerequisites for this phase. The current GX FIFO bridge is intentionally a sink, so a black screen is expected while CPU/runtime bring-up progresses.

## Validated runtime foundation

The following pieces are validated through CI and/or real Switch hardware:

1. Horizon lifecycle and SD filesystem services;
2. Wii guest-memory initialization and GuestFlat mapping;
3. WiiCompiled `HostContext` context-switch backend on AArch64;
4. generated data-section initialization;
5. translated execution handoff into PAL `__start` (`0x800060A4`);
6. translated direct/indirect dispatch integration;
7. host exception diagnostics with guest-context attribution;
8. durable unsupported-dispatch diagnostics;
9. headless platform initialization that bypasses the unrelated PrintConsole/NV framebuffer path;
10. early Wii SDK cache/timing/interrupt/exception HLE;
11. EXI/SI startup coverage and basic transaction HLE;
12. SD-backed NAND bootstrap/open/read/close semantics needed by the observed startup path;
13. DVD/ESP startup boundaries required before `main`;
14. PAL `main()` reached on real Switch hardware after 605 translated dispatches;
15. post-main execution through `System::RKSystem::main` (`0x80008EF0`) and `System::RKSystem::initialize` (`0x80009194`);
16. MEM2/allocator state needed by the second post-main `OSInitAlloc`;
17. mutex/thread/scheduler progression through `OSLockMutex`, `OSCreateThread`, `OSResumeThread`, `SelectThread`, `OSLoadContext`, `OSReceiveMessage`, and `OSSleepThread`;
18. HostContext-backed guest `OSThread` continuation that resumes an interior translated continuation rather than requiring a fake function entry;
19. VI/GX bootstrap state through `VISetPostRetraceCallback` while graphics output remains headless;
20. the observed WPAD initialization/getter/motor boundaries;
21. `PADInit` hardware-crossed far enough to expose `OSGetTime`;
22. `OSGetTime` hardware-crossed far enough to expose `OSSetPowerCallback`;
23. the current `OSSetPowerCallback` bridge, merged/pending hardware validation after this change.

## Current translated path

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
PAL __start (0x800060A4)
  ↓
early Wii SDK / OS / NAND / DVD / VI startup
  ↓
PAL main (0x8000B6B0)                    ✅ hardware validated
  ↓
System::RKSystem::main / initialize      ✅ hardware validated
  ↓
post-main MEM2 / mutex / thread setup
  ↓
HostContext-backed guest OSThread switch ✅ hardware validated
  ↓
WPADInit → WPADGetDpdSensitivity
  ↓
WPADGetStatus → WPADControlMotor
  ↓
PADInit                                  ✅ hardware crossed
  ↓
OSGetTime (0x801AAD5C)                   ✅ hardware crossed
  ↓
OSSetPowerCallback (0x801AB75C)          ← current merged frontier
  ↓
next hardware-proven boundary
```

`main()` is therefore no longer a pending milestone. The current task is to hardware-cross `OSSetPowerCallback` and identify the next post-main boundary.

## Current hardware-driven method

For every new blocker:

1. run the current NRO on real Switch hardware;
2. capture a durable `fast-track-dispatch-blocker.txt` or attributable exception;
3. identify the exact PAL address;
4. inspect semantics at the exact pinned WiiCompiled revision;
5. implement only the behavior proven or required by that boundary;
6. add Nintendo-data-free synthetic/CI coverage;
7. run the five repository CI workflows;
8. merge only after all five are green;
9. update `README.md`, `ROADMAP.md`, this document, the dated hardware result, and issue #117;
10. repeat on hardware.

This prevents speculative scheduler, input, renderer, filesystem, or device behavior from entering the fast-track simply because a nearby upstream API exists.

## Current post-main frontier

The recent sequence is:

- `WPADInit` (`0x801BF5C4`) — crossed on hardware;
- `WPADGetDpdSensitivity` (`0x801C329C`) — crossed on hardware;
- `WPADGetStatus` (`0x801BF64C`) — crossed on hardware;
- `WPADControlMotor` (`0x801C0EC4`) — crossed on hardware;
- `PADInit` (`0x801AF2F0`) — crossed on hardware;
- `OSGetTime` (`0x801AAD5C`) — crossed on hardware;
- `OSSetPowerCallback` (`0x801AB75C`) — current bridge, next hardware validation pending.

At the pinned revision, `OSSetPowerCallback` takes the new callback from guest `r3` and uses the guest SDA base in `r13`. The callback slot is `r13 - 0x62B8`, the handler-active flag is `r13 - 0x62C0`, and `0x801ABC0C` is the SDK default callback. The HLE disables interrupts, reads the previous callback, installs the requested callback or the SDK default for a NULL request, marks the STM handler active, restores the previous interrupt state, and returns NULL when the previous callback was the SDK default (otherwise the previous callback address).

The real Wii implementation would involve STM/IOS event registration. Pinned WiiCompiled deliberately stubs that host-device registration while preserving guest-visible SDA state. The Switch bridge mirrors only that boundary and does not create a speculative power-button event source.

## HostContext guest continuation

A key post-main runtime result was proving that saved guest thread state cannot always be resumed by dispatching a translated function entry. Hardware exposed saved SRR0 `0x80238A78`, an interior continuation inside `EGG::ProcessMeter::__ct`.

The Switch runtime now gives guest `OSThread` instances host `HostContext` continuations. Real hardware subsequently resumed the original translated host stack past that interior point and returned from `HostContext::Switch`, proving the continuation architecture works for the observed path.

See `HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md` for the evidence.

## Headless graphics status

The current `GX_HLE_FIFO_Write8/16/32/Float/Burst` bridge remains a temporary sink. It consumes translated GX FIFO writes without presenting them to a real Switch graphics backend.

Consequences:

- a black screen does **not** imply translated CPU execution is stalled;
- the runtime can continue advancing post-main while the screen remains black;
- first-frame work belongs to M3, where the FIFO sink must be replaced by a real GX → Switch renderer/backend.

## DVD / resource boundary

The fast-track deliberately does not fabricate Nintendo FST/resource data. A real local DVD FST/data mapping from the user's own dump remains pending and should only be wired when the hardware path actually requires it.

## Diagnostics

The local fast-track writes technical diagnostics under:

```text
sdmc:/switch/WiiCompiled-Switch/
```

Primary files:

- `fast-track-progress.txt` — coarse fast-track state;
- `fast-track-heartbeat.txt` — translated-dispatch liveness snapshot;
- `fast-track-main-reached.txt` — durable proof that PAL `main` was dispatched;
- `fast-track-dispatch-blocker.txt` — first unsupported direct/indirect boundary;
- `fast-track-exception.txt` — libnx exception record with AArch64 and guest context.

The diagnostics intentionally record runtime state only; no Nintendo game data or generated translated code is committed.

## Local build path

For a clean local hardware build:

```sh
git checkout main
git pull
git submodule update --init --recursive
MKW_JOBS=4 bash scripts/build-local-fast-track.sh
```

For repeated local rebuilds:

```sh
MKW_JOBS=4 bash scripts/build-local-fast-track-incremental.sh
```

The user-owned game inputs and generated translated product remain local-only. Do not commit or upload DOL/REL inputs, disc images, generated game-derived C++/objects, game-containing NRO/ELF files, keys, firmware, or extracted copyrighted assets.

## Public CI boundary

Public CI stays Nintendo-data-free. It validates platform/runtime code and synthetic execution seams with fabricated control probes, not Mario Kart Wii data.

Fast-track changes are expected to pass exactly these five workflows:

- `lint`;
- `fast-track-startup`;
- `stateful-translated-sequence`;
- `bootstrap-register-prelude`;
- `build-switch`.

## Next slices

1. build the current `main` local fast-track and run it on hardware;
2. verify that `OSSetPowerCallback` no longer appears as the first unsupported boundary;
3. capture the next blocker or attributable exception;
4. map that exact PAL address against pinned WiiCompiled;
5. implement only its verified semantics and repeat the CI/hardware cycle;
6. publish real local DVD/FST data only when resource loading proves it is required;
7. begin the real GX → Switch graphics backend when the pre-graphics runtime path is stable enough to make first-frame work meaningful.

## Evidence index

Use `ROADMAP.md` as the authoritative current checklist. Key evidence includes:

- `HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`;
- `HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md`;
- `HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md`;
- `HARDWARE_RESULTS_2026-09-16_WPAD_INIT.md`;
- `HARDWARE_RESULTS_2026-09-16_WPAD_DPD_SENSITIVITY.md`;
- `HARDWARE_RESULTS_2026-09-16_WPAD_GET_STATUS.md`;
- `HARDWARE_RESULTS_2026-09-16_WPAD_CONTROL_MOTOR.md`;
- `HARDWARE_RESULTS_2026-09-16_PAD_INIT.md`;
- `HARDWARE_RESULTS_2026-09-16_OS_GET_TIME.md`;
- `HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md`.

Older dated hardware result files are historical snapshots and intentionally retain the frontier wording that was true when each run was captured.
