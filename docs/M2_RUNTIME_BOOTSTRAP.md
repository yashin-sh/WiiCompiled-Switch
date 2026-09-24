# M2 — Horizon runtime bootstrap / translated fast-track

Status: **core bootstrap, PAL `main()`, guest thread continuation, sustained post-main execution, local RMCP01 FST publication, first local RMCP01 boot-resource read, VI-only AsyncDisplay idle recovery, complete `English.szs` SZS expansion, and the rendered fast-track runtime are hardware-validated on Nintendo Switch through 2026-09-24; the current gate is `GXInitTexObj (0x801707F8)` on image data inside the decompressed boot resource.**

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Maintain a stable WiiCompiled runtime on Horizon, link a locally generated translated Mario Kart Wii product, and advance real hardware execution from concrete evidence until game/resource initialization is complete enough to support the first rendered frame.

The original M2 goal of reaching PAL `main()` (`0x8000B6B0`) is complete. The active work is the post-main fast-track tracked in issue #117.

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
17. mutex/thread/scheduler progression through `OSLockMutex`, `OSCreateThread`, `OSResumeThread`, `SelectThread`, `OSLoadContext`, `OSReceiveMessage`, `OSSleepThread`, and `OSWakeupThread`;
18. HostContext-backed guest `OSThread` continuation that resumes an interior translated continuation rather than requiring a fake function entry;
19. VI/GX bootstrap state through `VISetPostRetraceCallback` while graphics output remains headless;
20. observed WPAD/PAD initialization/getter/motor boundaries;
21. `PADInit`, `OSGetTime`, `OSSetPowerCallback`, and `SCGetProductArea` hardware-crossed;
22. `OSWakeupThread` hardware-crossed into sustained translated execution;
23. a real run of 37,148 translated dispatches total / 36,543 post-main without a new unsupported-dispatch abort;
24. a later run reaching 126,563 total / 125,958 post-main dispatches;
25. exact RMCP01 attribution of `0x8020FCD4` to `PostRetraceCallback` and `0x8024373C` to `EGG::Thread::start(void*)`;
26. callback `r3 = 0x365E`, proving the guest VI retrace value advanced to 13,918;
27. classification of the prolonged black-screen path as active translated/VI execution rather than a durable translated-thread stall;
28. an independent Horizon liveness watchdog that remains available for future stall attribution without mutating guest state.

## Current translated path

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
WPAD / PAD                              ✅ hardware crossed
  ↓
OSGetTime                              ✅ hardware crossed
  ↓
OSSetPowerCallback                     ✅ hardware crossed
  ↓
SCGetProductArea                       ✅ hardware crossed
  ↓
OSWakeupThread                         ✅ hardware crossed
  ↓
sustained translated execution         ✅ 126,563 total dispatches
  ↓
PostRetraceCallback                     ✅ 0x8020FCD4
  ↓
guest VI retrace value                  ✅ 13,918 (0x365E)
  ↓
active VI/display loop                  ✅ hardware classified
  ↓
isolated M3 first-frame spike #162      ← next graphics frontier
```

There is currently **no new exact unsupported HLE boundary to implement**. The prolonged black-screen state has now been hardware-classified as an active translated/VI loop. The next graphics task is the isolated #162 first-frame probe; the normal fast-track keeps its FIFO sink until that backend path is proven.

## Current hardware-driven method

For a concrete unsupported boundary:

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

When no new blocker appears, use the independent liveness watchdog rather than guessing. It samples the translated heartbeat from a separate Horizon thread and records whether dispatch state is still changing.

## Current post-main frontier

The recent hardware sequence is:

- `WPADInit` (`0x801BF5C4`) — crossed;
- `WPADGetDpdSensitivity` (`0x801C329C`) — crossed;
- `WPADGetStatus` (`0x801BF64C`) — crossed;
- `WPADControlMotor` (`0x801C0EC4`) — crossed;
- `PADInit` (`0x801AF2F0`) — crossed;
- `OSGetTime` (`0x801AAD5C`) — crossed;
- `OSSetPowerCallback` (`0x801AB75C`) — crossed;
- `SCGetProductArea` (`0x801B23A0`) — crossed;
- `OSWakeupThread` (`0x801AAAA4`) — crossed;
- sustained translated execution — first 37,148 total / 36,543 post-main, then 126,563 total / 125,958 post-main;
- sampled target `0x8020FCD4` — exact RMCP01 `PostRetraceCallback`;
- sampled guest PC `0x8024373C` — exact RMCP01 `EGG::Thread::start(void*)`;
- callback `r3 = 0x365E` — VI retrace value 13,918.

The run no longer returned automatically to hbmenu because it did not hit the previous unsupported-dispatch abort path. The retrace-valued callback sample proves that the black-screen runtime remained active. Because the GX FIFO is still a sink, black output remains expected and now points directly at the deferred graphics path rather than a CPU liveness failure.

## HostContext guest continuation

A key post-main runtime result was proving that saved guest thread state cannot always be resumed by dispatching a translated function entry. Hardware exposed saved SRR0 `0x80238A78`, an interior continuation inside `EGG::ProcessMeter::__ct`.

The Switch runtime now gives guest `OSThread` instances host `HostContext` continuations. Real hardware subsequently resumed the original translated host stack past that interior point and returned from `HostContext::Switch`, proving the continuation architecture works for the observed path.

See `HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md` for the evidence.

## Headless graphics status

The current `GX_HLE_FIFO_Write8/16/32/Float/Burst` bridge remains a temporary sink. It consumes translated GX FIFO writes without presenting them to a real Switch graphics backend.

Consequences:

- a black screen does **not** imply translated CPU execution is stalled;
- the runtime can continue advancing post-main while the screen remains black;
- reaching `EGG::AsyncDisplay` does **not** mean a frame has been rendered;
- first-frame work belongs to M3, where the FIFO sink must be replaced by a real GX → Switch renderer/backend.

## DVD / resource boundary

The fast-track does not fabricate Nintendo FST/resource data. The user's own RMCP01 FST is published into guest MEM2 at `0x97DC0000` and validated on hardware. The narrow local `DATA/files` DVD bridge is now also hardware-proven to service a real boot-resource read: `/Boot/Strap/eu/English.szs`, 299,969 bytes.

## Diagnostics

The local fast-track writes technical diagnostics under:

```text
sdmc:/switch/WiiCompiled-Switch/
```

Primary files:

- `fast-track-progress.txt` — coarse fast-track state;
- `fast-track-heartbeat.txt` — latest translated-dispatch liveness snapshot;
- `fast-track-heartbeat-history.txt` — independent watchdog history with `ACTIVE`/`STALE`, stale seconds, dispatch delta, target and selected guest registers;
- `fast-track-main-reached.txt` — durable proof that PAL `main` was dispatched;
- `fast-track-dispatch-blocker.txt` — first unsupported direct/indirect boundary;
- `fast-track-exception.txt` — libnx exception record with AArch64 and guest context.

The watchdog is diagnostics-only. It never mutates guest CPU, memory, scheduler, or GX state.

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

1. preserve the current #117 runtime path as the hardware-validated active baseline;
2. proceed with isolated graphics spike #162;
3. prove Horizon clear-frame and triangle presentation through the candidate Dawn/Vulkan/NVK path;
4. feed fabricated Nintendo-data-free GX/FIFO traffic through pinned WiiCompiled `HleFifoWrite`;
5. measure Tegra X1 CPU overhead, memory use and frame pacing before choosing the backend;
6. only then connect the private local RMCP01 GX stream;
7. if a new runtime blocker/exception appears, return to the exact-address/pinned-semantics workflow;
8. keep the real local FST/DVD path hardware-driven: FST publication and the first `/Boot/Strap/eu/English.szs` read are proven; extend semantics only when the game reaches a new read/resource boundary.

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
- `HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md`;
- `HARDWARE_RESULTS_2026-09-16_SC_GET_PRODUCT_AREA.md`;
- `HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md`;
- `HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`;
- `HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md`.

Older dated hardware result files are historical snapshots and intentionally retain the frontier wording that was true when each run was captured.

## 2026-09-24 decodeSZS frontier

The VI-only SelectThread idle correction is hardware-crossed. The default
thread leaves the AsyncDisplay sync queue, becomes READY, resumes, and the
rendered path recovers real FIFO work plus a successful GPU present.

The same run keeps the TaskThread/DVD invariants intact and then stops at:

```text
DIRECT 0x80218C2C
r3 = 0x94226C20
r4 = 0x80F10300
```

RMCP01 maps that address to `EGG::Decomp::decodeSZS`; `r3` is exactly the
buffer filled by the successful `/Boot/Strap/eu/English.szs` read. The
current candidate mirrors only the pinned native Yaz0/SZS decoder.

## 2026-09-24 GXInitTexObj frontier

The pinned `EGG::Decomp::decodeSZS (0x80218C2C)` boundary is now
hardware-crossed:

```text
status       = decode-pass
src          = 0x94226C20
dst          = 0x80F10300
expand_size  = 2627200
src_consumed = 299969
dst_produced = 2627200
```

The next exact blocker is `GXInitTexObj (0x801707F8)`:

```text
obj       = 0x901136B4
image_ptr = 0x80F103E0
width     = 832
height    = 456
```

The image pointer lies inside the freshly decompressed `English.szs`
resource. The candidate consumes live r7-r10 for format/wrap/mipmap and ports
only this exact pinned texture-object initialization boundary.
