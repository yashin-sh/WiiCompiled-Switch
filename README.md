# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Project progress

**First game-facing RMCP01 GPU present: ✅ hardware validated**

**Estimated progress toward first visually confirmed Mario Kart Wii image: ~90%**

```text
██████████████████░░ 90%
```

> The previous “toward first rendered frame” estimate is retired because a real RMCP01 frame with `hadWork=1` has now successfully crossed `GXCopyDisp` and `g_surface.Present()` on hardware. The remaining percentage tracks visual/game-content confirmation rather than GPU viability: the present is proven, but the logs alone do not prove that the displayed pixels already form a visually correct Mario Kart Wii image.

## Current status

The project executes real WiiCompiled-translated Mario Kart Wii code on real Switch hardware and has **reached PAL `main()` (`0x8000B6B0`) after 605 translated dispatches**.

The post-`main` fast-track tracked in issue #117 has now hardware-crossed the observed path through guest thread/context switching, VI/GX bootstrap, WPAD/PAD initialization, timing, power-callback state, console-area lookup, and `OSWakeupThread`.

The rendered fast-track has since advanced far beyond that early sustained-retrace milestone. The latest accepted 2026-09-25 hardware evidence proves the following boot/resource/render chain:

- user-owned FST published at `0x97DC0000` (64,224 bytes / 2,096 entries);
- real `/Boot/Strap/eu/English.szs` DVD read completed with 299,969 bytes;
- pinned `EGG::Decomp::decodeSZS (0x80218C2C)` expanded it to 2,627,200 bytes;
- VI/post-retrace wakes the sleeping AsyncDisplay/default thread and execution resumes;
- real RMCP01 FIFO work reaches Aurora/Dawn/NVK and a game-facing present succeeds;
- pinned `GXInitTexObj (0x801707F8)` is hardware-crossed with `status=init-pass` for the observed 832x456 boot texture;
- the following exact IOS request is `/dev/net/kd/request`, mode 0.

PR #232's exact `IOS_Open (0x801938F8)` bridge is now **hardware-crossed**:
`/dev/net/kd/request`, mode 0, returns `fd=2000` and execution durably
continues into the next IOS boundary.

PR #235's first KD command-2 Boot probe is now **hardware-crossed**:
`fast-track-ios-ioctl-kd-cmd2.txt` reports `cmd2-boot-probe-pass`, and
execution durably advances to a new direct boundary.

PR #236's exact `IOS_Close (0x80193AD8)` bridge is now **hardware-crossed**:
`fast-track-ios-close-kd-request.txt` reports `close-pass` for fd 2000 and
execution durably advances far beyond the IOS boundary.

The same run reaches `RKSystem::run`, services a second real DVD read for
`/rel/StaticR.rel` (4,903,876 bytes), and continues through another round of
GX state setup. The exact blocker remains `GXLoadTexObj (0x80170F2C)` with
`oa=0x901136B4`, `tid=0`, but the merged diagnostics now capture the full
32-byte descriptor: 832x456, format 4, clamp/clamp, no mipmaps, backing
`0x00F103E0`. That exact texture load and the observed `GXSetTexCoordGen2 (0x8016E37C)`
identity/default tuple are now hardware-crossed. The following merged
`StrapScene::CheckInput (0x800077C8)` candidate is also crossed: the next run
continues to 17,800 translated dispatches, 765 RMCP01 FIFO writes and 61
successful presents with zero failures before reaching the first exact
StaticR.rel prolog boundary. The current blocker is
`INDIRECT_CALL_MISS 0x8055531C`, identified by the pinned map as
`RelProlog`, with the observed module base `r3=0x805102E0`.

```text
PAL main / post-main runtime                                       ✅ hardware crossed
  ↓
TaskThread → local DVD read of English.szs                         ✅ hardware crossed
  ↓
AsyncDisplay VI idle wake                                          ✅ hardware crossed
  ↓
EGG::Decomp::decodeSZS                                             ✅ hardware crossed
  ↓
real RMCP01 FIFO → GXCopyDisp → successful GPU present             ✅ hardware crossed
  ↓
GXInitTexObj (0x801707F8)                                          ✅ hardware crossed
  ↓
IOS_Open /dev/net/kd/request → fd 2000                             ✅ hardware crossed
  ↓
IOS_Ioctl (0x80194290), fd 2000, cmd 2                             ✅ hardware crossed
  in=0x80356F20/0x20 out=0x80356F40/0x20; reply word=-42, r3=0
  ↓
IOS_Close (0x80193AD8), fd 2000                                    ✅ hardware crossed
  ↓
/rel/StaticR.rel local DVD read                                    ✅ hardware crossed
  4,903,876 bytes; RKSystem::run reached
  ↓
GXLoadTexObj (0x80170F2C), oa=0x901136B4 tid=0                     ✅ hardware crossed
  832x456 RGB565, clamp/clamp, no mipmaps, data=0x00F103E0
  ↓
GXSetTexCoordGen2 (0x8016E37C)                                     ✅ hardware crossed
  TEXCOORD0 / MTX2x4 / TEX0 / IDENTITY / false / PTIDENTITY
  ↓
StrapScene::CheckInput (0x800077C8)                                ✅ hardware crossed
  scenePtr=0x90112A34; pinned guest-visible result r3=1
  ↓
StaticR RelProlog (0x8055531C)                                     ✅ hardware crossed
  269 StaticR dispatches; durable later DOL/OS execution
  ↓
OSDetachThread (0x801AA4EC)                                        ✅ hardware crossed
  TaskThread 0x901187C0; WAITING state 4; attr=1; empty join queue
  ↓
OSCancelThread (0x801AA1D4)                                        ✅ hardware crossed
  TaskThread termination completed; durable later UI/GX execution
  ↓
GXInitTexObjLOD (0x80170A4C)                                       🟡 exact candidate; hardware validation pending
  obj=0x9018E120; min/mag=1/1; min/max/bias=0/0/0; bc/el/aniso=0/0/0
  ↓
next exact hardware-attributed graphics/resource/game frontier     ⬜ pending
  ↓
visually confirmed Mario Kart Wii image                            ⬜ pending
```

The complete blocker-by-blocker history and current checklist live in [`ROADMAP.md`](ROADMAP.md). Hardware evidence is recorded in dated files under [`docs/`](docs/).

## Important limitations

### Graphics

The normal fast-track GX FIFO bridge remains intentionally a sink, so it stays a reliable **headless control baseline**. The separate rendered fast-track now has hardware-proven real RMCP01 FIFO work **and a successful game-facing `GXCopyDisp → g_surface.Present()`** with `hadWork=1`. Renderer viability and the first GPU present are therefore proven. The remaining graphics question is visual/game-content correctness and the later game/resource path, not whether Aurora/Dawn/NVK can present RMCP01 work on Switch.

### Filesystem / DVD

The project does not fabricate Nintendo game data. The user's own RMCP01 `DATA/sys/fst.bin` is hardware-proven to publish into guest MEM2 at `0x97DC0000`, and the narrow local `DATA/files` DVD bridge is now hardware-proven to service a real boot resource read: `/Boot/Strap/eu/English.szs`, 299,969 bytes. This validates the FST/file mapping on the current boot path; broader DVD semantics remain incomplete and continue to be added only when hardware reaches them.

### IOS / network

The first observed IOS network request is `/dev/net/kd/request`, mode 0; its fd-2000 open, first KD command-2 Boot probe, and fd-2000 close are all hardware-crossed. No broader IOS/network support is claimed: other closes, command 1/3, repeated command 2, ioctlv, NCD, IP, SSL, DNS, sockets, and online play remain outside the observed path.

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

For rendered RMCP01 work, these five checks are **necessary but not sufficient**. The public `build-switch` workflow now also compiles every rendered HLE bridge with `MKW_LOCAL_RENDERED_FAST_TRACK=1` against the pinned WiiCompiled/Aurora headers, which catches rendered-only C++ regressions before merge. The private `scripts/build-local-rendered-fast-track.sh` build remains a required sixth gate because public CI still cannot include the user-owned generated RMCP01 product or the complete private rendered link graph. A boundary is only called **hardware-crossed** when its hit count is non-zero **and** execution durably progresses beyond that target; a hit counter alone is not a PASS. See [`docs/FAST_TRACK_VALIDATION_POLICY.md`](docs/FAST_TRACK_VALIDATION_POLICY.md).

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
- [`docs/HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md) — hardware-crosses `GXFlush` with 23 successful presents and records the current TaskThread job-dispatch diagnostic frontier;
- [`docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md) — proves the received TaskThread “job” aliases the worker stack and moves the frontier to send-side / queue-buffer source attribution;
- [`docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md) — proves `TaskThread::request` sends the valid `mJobs[0]` pointer and moves the frontier to the exact `OSReceiveMessage` output-slot clobber phase;
- [`docs/HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md`](docs/HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md) — phase trace proves the receive slot is correct until the `OSWakeupThread` call boundary; attributes the clobber to Switch pre-call VI retrace delivery while guest interrupts are disabled and defines the minimal interrupt-mask fix;
- [`docs/HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md) — hardware-validates the interrupt-mask fix, records the first real `/Boot/Strap/eu/English.szs` read-pass, and moves the frontier to `SELECTTHREAD_IDLE_POLL`;
- [`docs/HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md) — attributes the idle blocker to `AsyncDisplay::syncTick` and the required wake to VI `PostRetraceCallback`, defining the VI-only scheduler idle candidate;
- [`docs/HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md) — hardware-validates AsyncDisplay idle recovery, recovers the real GPU present path, and moves the frontier to pinned `EGG::Decomp::decodeSZS (0x80218C2C)`;
- [`docs/HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md) — hardware-validates complete `English.szs` Yaz0 expansion and moves the exact frontier to `GXInitTexObj (0x801707F8)`;
- [`docs/HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md) — hardware-crosses `GXInitTexObj`, preserves one successful GPU present, and moves the exact frontier to `NAND_IOS_Open (0x801938F8)` with path diagnostics only;
- [`docs/HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md) — identifies the exact IOS path as `/dev/net/kd/request`, mode 0, and defines the minimal pinned device-handle open candidate;
- [`docs/HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md) — hardware-crosses the KD open, captures the full fd/cmd/in/out tuple for `IOS_Ioctl (0x80194290)` command 2, and defines the one-shot Boot-phase `-42` reply candidate;
- [`docs/HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md) — hardware-crosses the first KD command-2 Boot probe and moves the exact frontier to `IOS_Close (0x80193AD8)` with fd 2000;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_CTRL_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_CTRL_FRONTIER.md) — merged #203 hardware-proves `GXSetChanMatColor`, preserves the scheduler/GX chain, and exposes PAL `GXSetChanCtrl`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md) — real Switch progresses beyond `GXSetChanCtrl` and exposes PAL `GXSetNumTexGens`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md) — real Switch progresses beyond `GXSetNumTexGens` and exposes PAL `GXSetNumIndStages`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md) — real Switch progresses beyond `GXSetNumIndStages`, advances FIFO state traffic, and exposes PAL `GXSetNumTevStages`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md) — real Switch progresses beyond `GXSetNumTevStages` and exposes PAL `GXSetTevOp`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md) — real Switch progresses beyond `GXSetTevOp` and exposes PAL `GXSetTevOrder`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md) — real Switch progresses beyond `GXSetTevOrder` and exposes PAL `GXSetBlendMode`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md`](docs/HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md) — real Switch progresses beyond `GXSetBlendMode` and exposes PAL `GXSetColorUpdate`;
- [`docs/M3_RMCP01_RENDERED_FAST_TRACK.md`](docs/M3_RMCP01_RENDERED_FAST_TRACK.md) — first local Mario Kart graphics-enabled fast-track.

Older dated `HARDWARE_RESULTS_*` files are historical snapshots. Their “next blocker” wording intentionally reflects what was known on that date and is not rewritten retroactively.

## Upstream

The audited WiiCompiled revision is pinned to:

```text
a135beb201042b20f390c6695ca6b26768820fb4
```

CI enforces the pin. New hardware blockers are mapped against that exact revision before any HLE behavior is added.
