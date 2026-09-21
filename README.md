# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Project progress

**Estimated progress toward first rendered Mario Kart Wii frame: ~65%**

```text
█████████████░░░░░░░ 65%
```

> This percentage is an engineering estimate, not a function-count metric. The runtime has crossed PAL `main()` and sustained post-main VI/thread execution, while the complete Nintendo-data-free graphics chain through pinned `HleFifoWrite → Aurora GX → Dawn/Vulkan/NVK` is now hardware-proven. The remaining gap is the first real local RMCP01 frame and the game-facing blockers it exposes.

## Current status

The project executes real WiiCompiled-translated Mario Kart Wii code on real Switch hardware and has **reached PAL `main()` (`0x8000B6B0`) after 605 translated dispatches**.

The post-`main` fast-track tracked in issue #117 has now hardware-crossed the observed path through guest thread/context switching, VI/GX bootstrap, WPAD/PAD initialization, timing, power-callback state, console-area lookup, and `OSWakeupThread`.

The latest hardware run confirms the prolonged black-screen path is **actively executing**, not sitting at a durable translated-thread stall. It reached **126,563 translated dispatches total, including 125,958 after `main()`**. RMCP01 maps the sampled target `0x8020FCD4` exactly to `PostRetraceCallback`, while guest PC `0x8024373C` is `EGG::Thread::start(void*)`.

The sampled callback carried `r3 = 0x365E` (**13,918**). The Switch VI bridge sets `r3` to the new retrace value immediately before invoking the post-retrace callback, so this is direct evidence that the VI/retrace loop continued advancing for thousands of retraces.

The normal #117 fast-track deliberately keeps its FIFO sink as a stable headless control baseline. Separately, M3 has hardware-validated native Vulkan, Dawn/WebGPU, Aurora GX and the exact pinned WiiCompiled `HleFifoWrite` path; the synthetic decoder run remained active for 1,435 frames. The rendered RMCP01 target is running on hardware, its user-owned FST is published successfully, and #185 local DVD reads are installed but not reached. #186 identified the durable priority-6 OSThread as `0x90112660` with virtual `run()` `0x80008D18`.

The latest 2026-09-21 rendered real-Switch run progresses beyond merged #206's PAL `GXSetNumTexGens (0x8016E5A4)` bridge and exposes the distinct next DIRECT blocker `GXSetNumIndStages (0x80171B38)` with captured `r3 = 0`. Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` implements that boundary as `GXSetNumIndStages((u8)r3)`. The final blocker records stage `RMCP01_GX_SET_NUM_TEX_GENS`, proving progression past the previous bridge. The renderer/FST path remains valid; the pre-blocker snapshot still records nine FIFO writes with no display list, drawable FIFO work, `GXCopyDisp`, or present. The next patch is restricted to this exact `GXSetNumIndStages` boundary.

## Milestones

| Milestone | Status |
| --- | --- |
| Native Horizon/libnx runtime boots | ✅ Done |
| WiiCompiled PPC → AArch64 translated code executes on Switch | ✅ Done |
| Guest memory/data initialization | ✅ Done |
| HostContext / GuestFlat / translated handoff | ✅ Done |
| Reach Mario Kart Wii `main()` | ✅ Hardware validated |
| Enter post-`main` game initialization | ✅ Hardware validated |
| Thread/context continuation across guest `OSThread` switches | ✅ Hardware validated |
| Observed post-main OS/VI/WPAD/PAD/time/power/SC boundaries | ✅ Hardware validated |
| `OSWakeupThread` scheduler handoff | ✅ Hardware validated |
| Sustained post-main translated execution | ✅ Hardware validated |
| Classify active loop vs durable stall | ✅ Active VI/retrace loop confirmed |
| Game/resource initialization | 🟡 In progress |
| Native Switch GPU clear/present (NVK/VI) | ✅ Hardware validated |
| Native Vulkan triangle / shader pipeline (NVK/VI) | ✅ Hardware validated |
| Dawn/WebGPU → Vulkan/NVK clear/present | ✅ Hardware validated (1,507-frame loop) |
| Dawn WGSL triangle / graphics pipeline | ✅ Hardware validated + clean exit |
| Aurora GX triangle | ✅ Hardware validated (563-frame active loop) |
| WiiCompiled FIFO → Aurora GX | ✅ Hardware validated (1,435-frame loop) |
| RMCP01 rendered fast-track | ✅ Running on hardware; renderer ready, no drawable RMCP01 work yet |
| WiiCompiled/Aurora GX → first RMCP01 frame | 🟡 M3 #162 in progress |
| Input/audio/filesystem completeness and gameplay | ⬜ Pending |

## Current fast-track path

```text
headless Horizon platform init
  ↓
PAL __start (0x800060A4)
  ↓
early Wii SDK / OS / NAND / DVD / VI bring-up
  ↓
PAL main() (0x8000B6B0)                    ✅ reached on hardware
  ↓
System::RKSystem::main / initialize        ✅ reached on hardware
  ↓
post-main MEM2 / mutex / thread / VI
  ↓
HostContext-backed guest OSThread switch   ✅ hardware validated
  ↓
WPAD / PAD initialization                  ✅ hardware crossed
  ↓
OSGetTime (0x801AAD5C)                     ✅ hardware crossed
  ↓
OSSetPowerCallback (0x801AB75C)            ✅ hardware crossed
  ↓
SCGetProductArea (0x801B23A0)              ✅ hardware crossed
  ↓
OSWakeupThread (0x801AAAA4)                ✅ hardware crossed
  ↓
sustained post-main translated execution   ✅ 126,563 total dispatches
  ↓
PostRetraceCallback (0x8020FCD4)            ✅ repeatedly reached
  ↓
VI retrace value 0x365E / 13,918            ✅ active loop confirmed
  ↓
NVK/VI clear-frame presentation             ✅ hardware validated
  ↓
Vulkan triangle                              ✅ hardware validated
  ↓
Dawn/WebGPU clear/present                     ✅ hardware validated
  ↓
Dawn WGSL triangle                             ✅ hardware validated + clean exit
  ↓
Aurora GX triangle                             ✅ hardware validated
  ↓
HleFifoWrite synthetic FIFO                     ✅ hardware validated, 1,435 frames
  ↓
RMCP01 rendered fast-track                      ✅ hardware running
  ↓
local FST publication @ 0x97DC0000              ✅ hardware validated
  ↓
later OSThread 0x90112660 / run 0x80008D18       ✅ identified
  ↓
#188 first-fiber VI poll register clobber             ✅ fixed + hardware revalidated
  ↓
0x90112660 VI wait-queue starvation                    ✅ hardware fixed
  ↓
OSSendMessage (0x801A735C)                             ✅ hardware crossed
  ↓
GXDrawDone (0x8016EAB0)                                 ✅ hardware crossed
  ↓
EGG::TaskThread::run (0x80242D7C)                         ✅ hardware crossed
  ↓
GXSetProjection (0x8017301C)                               ✅ hardware crossed
  ↓
GXSetViewport (0x801733B4)                                 ✅ hardware crossed
  ↓
GXSetScissor (0x80173430)                                  ✅ hardware crossed
  ↓
GXLoadPosMtxImm (0x8017310C)                                ✅ hardware crossed
  ↓
GXSetCurrentMtx (0x80173214)                                ✅ hardware crossed
  ↓
GXClearVtxDesc (0x8016DC34)                                 ✅ hardware crossed
  ↓
GXInvalidateVtxCache FIFO byte 0x48                          ✅ first post-bootstrap GX state traffic
  ↓
GXSetVtxDesc (0x8016D3A4)                                   ✅ hardware crossed
  ↓
GXSetVtxAttrFmt (0x8016DC68)                                 ✅ hardware crossed
  ↓
GXSetNumChans (0x8017054C)                                   ✅ hardware crossed
  ↓
GXSetChanMatColor (0x80170474)                                ✅ hardware crossed
  ↓
GXSetChanCtrl (0x80170570)                                    🟡 implemented in #204; hardware validation pending
  ↓
resource-job callback / next GX/DVD frontier
  ↓
first rendered RMCP01 frame
```

The complete blocker-by-blocker history and current checklist live in [`ROADMAP.md`](ROADMAP.md). Hardware evidence is recorded in dated files under [`docs/`](docs/).

## Important limitations

### Graphics

The normal fast-track GX FIFO bridge remains intentionally a sink, so it stays a reliable **headless control baseline**. The separate rendered fast-track now uses the hardware-proven `HleFifoWrite → Aurora GX → Dawn/WebGPU → Vulkan/NVK` path and presents at the RMCP01 `GXCopyDisp` boundary. The next unknown is therefore game-facing behavior, not renderer viability.

### Filesystem / DVD

The project does not fabricate Nintendo game data. The user's own RMCP01 `DATA/sys/fst.bin` is now hardware-proven to publish into guest MEM2 at `0x97DC0000`, and the narrow `DATA/files` DVD read bridge is installed. The current startup path has not yet reached `DVDReadPrio` / `DVDReadAsyncPrio`, so no claim of real file-read hardware PASS is made yet.

### Input

Some pinned WPAD/PAD initialization boundaries are mirrored because hardware reached them, but full Joy-Con / Pro Controller / Wii Remote / GameCube input semantics are **not** implemented yet. Input behavior is added only when hardware evidence proves the required boundary and pinned semantics.

## Local fast-track hardware test

After pulling `main`:

```sh
git checkout main
git pull
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-local-fast-track.sh
```

For repeat local rebuilds, the incremental helper also exists:

```sh
MKW_JOBS=4 bash scripts/build-local-fast-track-incremental.sh
```

Copy `WiiCompiled-Switch-local-fast-track.nro` to the Switch and launch it through hbmenu in application/title-override mode with full memory.

For the first graphics-enabled RMCP01 run, keep that headless NRO as the control baseline and build the separate rendered target:

```sh
MKW_JOBS=4 bash scripts/build-local-rendered-fast-track.sh
```

This produces `WiiCompiled-Switch-local-rendered-fast-track.nro`. It contains locally generated game-derived code and must not be uploaded or committed.

The fast-track is intentionally headless. Use the SD diagnostic files instead of expecting a text console:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-thread-events.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat-history.txt
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

For future prolonged runs, `fast-track-heartbeat-history.txt` remains the strongest stall diagnostic. The 2026-09-18 hardware result additionally proves active VI progression from the callback's guest retrace value itself: `r3 = 0x365E` at `PostRetraceCallback`.

## Public CI boundary

Public CI remains Nintendo-data-free. A real WiiCompiled Mario Kart Wii product is generated locally from a user-owned dump before the Switch build and linked into the NRO. Generated game-derived C++/objects/data and game-containing NRO/ELF artifacts are never committed or uploaded by public CI.

The repository currently validates five Nintendo-data-free CI workflows for fast-track changes:

- `lint`;
- `fast-track-startup`;
- `stateful-translated-sequence`;
- `bootstrap-register-prelude`;
- `build-switch`.

For rendered RMCP01 work, these five checks are **necessary but not sufficient**: the private `scripts/build-local-rendered-fast-track.sh` build is a required sixth gate because public CI cannot include the user-owned generated RMCP01 product or the complete private rendered link graph. A boundary is only called **hardware-crossed** when its hit count is non-zero **and** execution durably progresses beyond that target; a hit counter alone is not a PASS. See [`docs/FAST_TRACK_VALIDATION_POLICY.md`](docs/FAST_TRACK_VALIDATION_POLICY.md).

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, decrypted content, or generated translated game output**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See [`LEGAL.md`](LEGAL.md).

## Requirements

- A Nintendo Switch capable of running homebrew under Atmosphère
- Homebrew Menu (`hbmenu`)
- devkitPro with `devkitA64` and `libnx`
- GNU Make

Initialize the pinned WiiCompiled source and build the Nintendo-data-free runtime probe:

```sh
git submodule update --init --recursive
make
```

Expected public probe output:

```text
WiiCompiled-Switch.nro
```

The public probe contains no translated Mario Kart Wii product. Game-derived translation is a separate local-only build path.

## Current architecture

```text
user-owned Wii dump (local build only)
            |
            v
WiiCompiled translator
            |
            v
generated C++ / RuntimeConfig / data init
            |
            +------ linked at build time ------+
                                              |
                                              v
+----------------------------------------------------------+
| Native AArch64 NRO                                      |
| WiiCompiled translated product + runtime                |
| Switch platform adapter                                 |
| - lifecycle / applet                                    |
| - runtime filesystem + diagnostics / NAND backing       |
| - input HLE state (partial, hardware-driven)            |
| - audio (bootstrap/HLE incomplete)                      |
| - graphics (headless sink baseline + rendered variant)  |
| - context switching / timing / guest memory             |
| - independent fast-track liveness watchdog              |
+----------------------------------------------------------+
            |
            v
        libnx / Horizon
            |
            v
       Atmosphère / Switch
```

## Roadmap and evidence

Start with:

- [`ROADMAP.md`](ROADMAP.md) — authoritative current milestone/frontier checklist;
- [`docs/FAST_TRACK_VALIDATION_POLICY.md`](docs/FAST_TRACK_VALIDATION_POLICY.md) — required validation ladder, strict hardware-cross definition, invariant checklist, and private rendered-build gate;
- [`docs/M2_RUNTIME_BOOTSTRAP.md`](docs/M2_RUNTIME_BOOTSTRAP.md) — current runtime/bootstrap architecture and hardware method;
- [`docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`](docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md) — first real `main()` proof;
- [`docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md`](docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md) — HostContext guest continuation proof;
- [`docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md`](docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md) — scheduler frontier that preceded sustained execution;
- [`docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`](docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md) — first sustained black-screen / AsyncDisplay evidence;
- [`docs/HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md`](docs/HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md) — 126,563-dispatch run proving the black-screen path is an active VI/post-retrace loop;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md`](docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md) — real-Switch changing-color NVK/VI clear-frame presentation proof;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md`](docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md) — real-Switch Vulkan shader/pipeline/rasterisation triangle proof and SD-report follow-up;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md`](docs/HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md) — real-Switch Aurora GX triangle proof, 563-frame active loop, and clean teardown;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md`](docs/HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md) — real-Switch exact pinned `HleFifoWrite` → Aurora GX proof with a 1,435-frame active loop;
- [`docs/HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md) — FST publication PASS, #185 DVD-read non-reachability, and later priority-6 thread frontier;
- [`docs/HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md`](docs/HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md) — #188 first-fiber crash attribution and register-isolation fix gate;
- [`docs/HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md) — #189 hardware PASS, VI starvation fixed, and new `OSSendMessage` blocker;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md) — #190 OSSendMessage PASS and new `GXDrawDone` blocker;
- [`docs/HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md) — #191 GXDrawDone PASS and resource `TaskThread::run` frontier;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md) — first #192 hardware run, TaskThread validation caveat, and new PAL `GXSetProjection` blocker;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md) — #193 hardware-proves TaskThread + projection and exposes PAL `GXSetViewport`;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md) — #194 hardware-proves `GXSetViewport` and exposes PAL `GXSetScissor`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md) — #195 hardware-proves `GXSetScissor` and exposes PAL `GXLoadPosMtxImm`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md) — #196 hardware-proves `GXLoadPosMtxImm`, re-proves `TaskThread::run`, and exposes PAL `GXSetCurrentMtx`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md) — #197 hardware-proves `GXSetCurrentMtx` and exposes PAL `GXClearVtxDesc`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md) — #198 hardware-proves `GXClearVtxDesc`, re-proves TaskThread, records first post-bootstrap GX FIFO state traffic, and exposes PAL `GXSetVtxDesc`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md) — #199 hardware-proves `GXSetVtxDesc` and exposes PAL `GXSetVtxAttrFmt`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md) — #200 hardware-proves `GXSetVtxAttrFmt`, explains the deliberate abort/return-to-hbmenu behavior, and exposes PAL `GXSetNumChans`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md) — #201 hardware-proves `GXSetNumChans`, re-proves TaskThread, and exposes PAL `GXSetChanMatColor`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_CTRL_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_CTRL_FRONTIER.md) — merged #203 hardware-proves `GXSetChanMatColor`, preserves the scheduler/GX chain, and exposes PAL `GXSetChanCtrl`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md) — real Switch progresses beyond `GXSetChanCtrl` and exposes PAL `GXSetNumTexGens`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md) — real Switch progresses beyond `GXSetNumTexGens` and exposes PAL `GXSetNumIndStages`;
- [`docs/M3_RMCP01_RENDERED_FAST_TRACK.md`](docs/M3_RMCP01_RENDERED_FAST_TRACK.md) — first local Mario Kart graphics-enabled fast-track.

Older dated `HARDWARE_RESULTS_*` files are historical snapshots. Their “next blocker” wording intentionally reflects what was known on that date and is not rewritten retroactively.

## Upstream

The audited WiiCompiled revision is pinned to:

```text
a135beb201042b20f390c6695ca6b26768820fb4
```

CI enforces the pin. New hardware blockers are mapped against that exact revision before any HLE behavior is added.
