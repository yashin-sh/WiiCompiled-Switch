# M3 — local RMCP01 rendered fast-track

Tracking: #117, #162, #154, #4

Status: **renderer, real RMCP01 FIFO work, successful GPU presentation, local FST/DVD/SZS path, VI-only idle recovery, `GXInitTexObj`, PAL `GXFlush`, the KD open/cmd2/close sequence, `RKSystem::run`, and a real `StaticR.rel` read are hardware-proven. The current exact frontier is the first `GXLoadTexObj (0x80170F2C)`; its full descriptor is hardware-captured and an exact one-descriptor bind candidate awaits validation.**

## Purpose

This is the first graphics-enabled Mario Kart Wii fast-track. It deliberately
exists beside the hardware-stable headless #117 baseline instead of replacing
it, so any renderer regression can be compared against the known-good CPU/
scheduler path.

No generated game code, game assets, disc data or game-containing NRO belongs
in the public repository. The user's local translated RMCP01 product is linked
only on their machine.

## Path

```text
local RMCP01 WiiCompiled product
        ↓
translated GX gather-pipe stores
        ↓
GX_HLE_FIFO_Write8/16/32/Float/Burst
        ↓
pinned WiiCompiled HleFifoWrite
        ↓
Aurora GX
        ↓
Dawn/WebGPU
        ↓
Vulkan / loaderless NVK
        ↓
GXCopyDisp present boundary
        ↓
VK_NN_vi_surface / NWindow
        ↓
Switch display
```

The renderer initialization, FIFO decoder, Aurora GX layer, Dawn backend and
NVK presentation below the game have each already passed isolated hardware
gates.

## Deliberate differences from the headless baseline

- `source/gx_fast_track_fifo_bridge.cpp` is excluded only from this target;
- the real pinned `runtime/src/hle/gx/gx_fifo.cpp` is linked instead;
- the four FIFO write helpers forward to `HleFifoWrite`;
- the fast-track GXInit boundary additionally initializes Aurora GX;
- PAL `GXCopyDisp` `0x8016FC38` is a native rendered-only boundary that
  resolves and presents the current frame;
- the existing Switch VI/retrace bookkeeping remains in place;
- PrintConsole remains disabled.

The renderer pre-warms the next Aurora frame after every present, matching the
pinned runtime's requirement that CP/BP/XF commands emitted before the next draw
have an active frame.

## First-frame limitations

This first RMCP01 integration intentionally does not import the desktop
application layer, overlay, or full guest-write texture invalidation cache.
Those are added only if the real game path proves they are required.

Display-list execution is routed back through the same pinned
`GX_HLE_FIFO_WriteBurst` decoder from mapped guest memory. The stable
headless target remains available as the liveness/control baseline.

## Current resource gate — 2026-09-19

The rendered hardware heartbeat proved that the lower renderer is active but the
game still produces no drawable work. The eight observed FIFO writes classify
as the video-bootstrap BP registers `0x49`, `0x4A`, `0x4D` and `0x4E`; the last
word `0x4E000100` is the normal `GXSetDispCopyYScale(1.0)` state write.

The same run reported:

```text
RKSystem::run hits    : 0
StaticR dispatches    : 0
FST address           : 0x00000000
FST size              : 0x00000000
FST structurally valid: NO
FIFO produced work    : NO
GXCopyDisp calls      : 0
```

That satisfies the hardware gate tracked by #154. The rendered fast-track now
mirrors the pinned WiiCompiled guest-publication contract from the user's own
local extraction:

```text
/switch/WiiCompiled-Switch/DATA/
├── files/
└── sys/
    ├── boot.bin   # must identify RMCP01
    └── fst.bin
```

At `DVDInit`, the runtime validates `boot.bin` as `RMCP01`, validates the
big-endian FST header, copies `fst.bin` into the already-reserved 2 MiB MEM2
region below the IPC arena, publishes `0x80000038` / `0x8000003C`, then invokes
translated `__DVDFSInit`. Missing or invalid local data does not fabricate a
filesystem and leaves the previous safe path intact.

The publication result is written to:

```text
/switch/WiiCompiled-Switch/dvd-fst-status.txt
```

This slice intentionally does not implement arbitrary DVD file reads yet. The
next hardware run determines the exact read/REL boundary that must be added.

## Hardware result after local FST publication — 2026-09-19

The first hardware run after #183 proves the SD-backed FST publication path:

```text
status=published
address=0x97dc0000
size=64224
entries=2096
```

The FST is already structurally valid when PAL `main()` is reached. The same
run reaches `System::RKSystem::main`, `System::RKSystem::initialize`,
`EGG::Video::initialize` and `EGG::Video::configure`. The renderer remains
ready, but `System::RKSystem::run` and StaticR still have zero dispatches.

The eight RMCP01 FIFO writes remain exactly the video-configuration BP state
`0x49`, `0x4A`, `0x4D`, `0x4E`; there is still no display list, drawable FIFO
work, `GXCopyDisp` or presentation.

The durable liveness path then repeatedly reaches `PostRetraceCallback`
(`0x8020FCD4`) while guest PC remains `EGG::Thread::start(void*)`
(`0x8024373C`). This proves the VI/worker path remains active, but does not
prove that the default/main guest thread resumes and completes
`RKSystem::initialize()`.

The next diagnostic slice is therefore scheduler-focused and behavior-neutral.
It records:

- `VIWaitForRetrace`, `PostRetraceCallback`, `OSReceiveMessage`,
  `OSSleepThread`, `OSWakeupThread`, `SelectThread` and `OSLoadContext` hit counts;
- current guest-fiber and OS current/running thread pointers;
- default and active thread state/suspend/priority/queue fields;
- LR and guest-fiber identity in translated trace lines;
- the first 128 translated dispatches starting at `EGG::Video::configure` in
  `/switch/WiiCompiled-Switch/fast-track-post-video-trace.txt`.

No guest scheduler, DVD, REL or renderer behavior is changed by this diagnostic.

## Hardware result after #184 — DVD retry frontier\n\nThe scheduler-focused hardware run closes the earlier ambiguity:\n\n- the default/main thread (`0x80347498`) is still present and becomes `READY`\n  at priority 16 on run queue `0x80347830`;\n- guest thread `0x90112660` remains `RUNNING` at priority 6;\n- `VIWaitForRetrace`, `PostRetraceCallback`, and `OSWakeupThread` then advance\n  together at the sustained retrace cadence while `SelectThread` barely advances;\n- `RKSystem::run` and StaticR remain at zero;\n- the graphics path still contains only the eight `Video::configure` BP writes.\n\nThis disproves the idea that the default thread was simply lost. The higher-\npriority worker remains active while the default thread is runnable.\n\nSource attribution provides the next concrete gate: in pinned RMCP01,\n`EGG::DvdRipper::loadToMainRAM` is the explicit non-video path that retries a\nnegative `DVDRead` by calling `VIWaitForRetrace()` and trying again. Pinned\nWiiCompiled therefore native-overrides `DVDReadPrio` (`0x8015E834`) and the\ninternal `DVDReadAsyncPrio` (`0x8015E74C`) against the user-owned extracted\n`DATA/files` source.\n\nThe Switch port already publishes the local FST but did not yet provide those\nread overrides. The next hardware slice adds only that missing read contract:\nresolve `DVDFileInfo::startAddr` through the published FST, read the matching\n`DATA/files` payload into guest RAM, publish DVD completion state, and notify\nthe existing guest-RAM DMA seam. A bounded `dvd-read-status.txt` records the\nfirst 32 attempts for hardware attribution.\n\n## Hardware result after #185 — later prio-6 guest thread frontier

The first hardware run with the local DVD-read bridge disproves the previous
DVD-retry attribution for the current startup path:

- the FST remains published and structurally valid at `0x97DC0000`;
- neither `DVDReadPrio` (`0x8015E834`) nor internal
  `DVDReadAsyncPrio` (`0x8015E74C`) appears in the bounded post-video trace;
- no `dvd-read-status.txt` is produced, so #185 is not reached in this run;
- the default thread is still running after the initial ProcessMeter
  `OSReceiveMessage` / `OSSleepThread` handoff;
- only later, between roughly 2 and 3 seconds, execution switches durably to
  OSThread `0x90112660`, priority 6, while the default thread becomes READY;
- the sustained sample remains `PostRetraceCallback` at `0x8020FCD4` with
  guest PC `EGG::Thread::start(void*)` at `0x8024373C`.

The initial priority-6 ProcessMeter thread is not the durable blocker: its
OSThread is created at `0x8042A680`, whereas the sustained active OSThread is
`0x90112660`. The next hardware diagnostic therefore records bounded
`OSCreateThread -> OSResumeThread -> GuestFiberEntry` lifecycle events,
including entry point, entry argument, priority, EGG object vtable and the
`Thread::run` virtual slot. This should identify the exact later thread class
without changing scheduler or renderer semantics.

PR #186 is merged on `main` as
`1fa6a0b1395d429cc9dcae979154f76eeb32f2d4`. The new bounded hardware log is:

```text
/switch/WiiCompiled-Switch/fast-track-thread-events.txt
```

The next acceptance condition is an event for `thread=0x90112660` exposing
its entry argument, EGG vtable and especially the virtual `run()` slot. Until
that identity is known, do not change guest scheduling priority, DVD semantics
or the renderer.

The consolidated 2026-09-19 evidence is recorded in
`HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md`.

## Hardware result after #186 — VI guest-fiber starvation

The bounded thread lifecycle log identifies the durable worker exactly enough
to close the scheduler ambiguity:

```text
thread=0x90112660
entry=0x8024373c
arg=0x8042e930
prio=6
vtable=0x80270bc0
vt_run=0x80008d18
```

This worker is distinct from the initial ProcessMeter thread. At the end of the
same hardware run the worker remains RUNNING at priority 6 while the default
thread is READY at priority 16. Counters show 928 `VIWaitForRetrace` calls,
925 `OSWakeupThread` calls, but only 6 `SelectThread` calls.

The Switch `VIWaitForRetrace` bridge was still using pinned WiiCompiled's
non-fiber fallback: `svcSleepThread` paces the Horizon host thread but does
not place the guest OSThread on the VI wait queue. Once HostContext-backed guest
fibers exist, that leaves a higher-priority guest RUNNING across every retrace
and starves the default thread.

The next implementation slice therefore mirrors the pin's fiber path:
`OSSleepThread(0x80386BC0)` while waiting for the retrace count to change,
plus bounded synchronous time-driven retrace polling from safe runtime call
boundaries. It does not change guest priorities, DVD reads or renderer
semantics.

## Hardware result after #188 — runtime-boundary VI poll context regression

The first rendered hardware run after #188 does **not** reach the previously
identified durable worker `0x90112660`. Instead it stops much earlier while
entering the initial HostContext-backed guest OSThread:

```text
thread      : 0x8042A680
entry       : 0x8024373C  EGG::Thread::start(void*)
entry arg   : 0x804294E4
fiber r1    : 0x8042A658
fiber r3    : 0x804294E4
blocker kind: GUEST_FIBER_ENTRY_EXCEPTION
stage       : HOST_CONTEXT_SWITCH_ENTER
```

At the same point `RKSystem::run`, StaticR, `OSSleepThread` and `SelectThread`
are still zero. The renderer remains initialized and the FIFO remains at the
same eight video-bootstrap BP writes, so this is not a DVD or graphics
regression.

#188 added time-driven VI polling to every translated runtime call boundary.
That poll advances retraces through the same `CpuContext` that holds the next
callee's ABI arguments. `AdvanceRetrace` deliberately reuses `r3` for the VI
wait queue, `OSWakeupThread`, and pre/post-retrace callback arguments. The
pinned WiiCompiled runtime isolates interrupt-like retrace service from the
interrupted translated register file; the Switch call-boundary poll did not.

The minimal correction preserves and restores the **complete** interrupted
`CpuContext` around `mkw_switch_hle_vi_poll_retrace`. Guest memory, VI state,
wait queues and scheduler state remain shared, but callback/scheduler scratch
registers cannot leak into the translated callee. No guest priority, DVD,
Aurora, Dawn or GX semantics are changed.

Hardware acceptance now proceeds in two stages:

1. `0x8042A680` must again cross `EGG::Thread::start` and reach its historical
   `OSReceiveMessage -> OSSleepThread` path without `GUEST_FIBER_ENTRY_EXCEPTION`;
2. only then evaluate #188's original goal: `0x90112660` should park on VI queue
   `0x80386BC0`, with `OSSleepThread` / `SelectThread` increasing and the
   default thread getting real execution time between retraces.

Full evidence and attribution are recorded in
`HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md`.

## Hardware result after #189 — scheduler recovery and OSSendMessage frontier

The next real-Switch run validates both pending VI/scheduler gates:

- initial OSThread `0x8042A680` crosses `EGG::Thread::start` and returns
  through its historical blocking message path;
- later worker `0x90112660`, priority 6, enters its guest fiber and returns
  in WAITING state on VI queue `0x80386BC0`;
- final scheduler snapshot is back on the default/main thread
  `0x80347498`, priority 16;
- counters advance to `OSSleepThread=2`, `OSWakeupThread=318`,
  `SelectThread=27`.

The run reaches 2,581 translated dispatches / 1,975 post-main dispatches before
the next explicit blocker:

```text
kind   : DIRECT
target : 0x801A735C
r3     : 0x8042BBFC
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

At the pinned WiiCompiled revision, `0x801A735C` is `OSSendMessage`.
The renderer remains ready with the same eight bootstrap FIFO writes and no
display list / drawable work / `GXCopyDisp`; FST publication remains valid and
no DVD read is reached. The next implementation is therefore the pinned
message-queue send contract only.

Full evidence is recorded in
`HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md`.

## Hardware result after #190 — GXDrawDone frontier

The first real-Switch run after #190 crosses the previous PAL
`OSSendMessage (0x801A735C)` blocker and preserves the scheduler fixes:

- `0x8042A680` still returns through its blocking message path;
- priority-6 worker `0x90112660` still parks on VI queue `0x80386BC0`;
- default/main `0x80347498` remains the current/running thread in the durable
  snapshot;
- the liveness watchdog remains ACTIVE while translated dispatches advance.

The new explicit blocker is:

```text
kind   : DIRECT
target : 0x8016EAB0
r3     : 0x8042944C
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

At the pinned WiiCompiled revision, `0x8016EAB0` is `GXDrawDone`.
Pinned behavior clears the guest draw-done flag, drains GX, then publishes the
PE-finish bit and draw-done flag. In the rendered fast-track the implementation
uses Aurora's real `GXDrawDone()` FIFO drain and preserves the separate
`GXCopyDisp` present boundary.

The renderer remains initialized with exactly eight bootstrap FIFO writes and
still has no display-list call, drawable FIFO work, `GXCopyDisp`, or present.
FST publication remains valid; `RKSystem::run` and StaticR are still zero.

## Hardware result after #191 — TaskThread resource frontier

The first real-Switch run after #191 crosses `GXDrawDone (0x8016EAB0)`.
The next durable blocker is an `INDIRECT_JUMP_MISS` to
`0x80242D7C` from `EGG::Thread::start(void*)`.

The matching thread lifecycle event identifies:

```text
thread   : 0x8042E480
arg      : 0x8042BBF0
priority : 24
vtable   : 0x802A3F90
vt_run   : 0x80242D7C
```

Pinned WiiCompiled maps this target to `EGG::TaskThread::run()`. RMCP01's
`ResourceManager` creates a priority-24 `EGG::TaskThread` for asynchronous
resource jobs, so this is the first direct hardware evidence that the
resource-worker fiber itself is now being entered.

The renderer still reports eight bootstrap FIFO writes, no display list,
no drawable FIFO work, no `GXCopyDisp`, and no present. FST publication
remains valid and no DVD-read diagnostic is produced yet.

The next slice mirrors the pinned `TaskThread::run` loop and routes this
virtual native target before the generated translated indirect table.

## Hardware result after #192 — GXSetProjection frontier

The first real-Switch run after #192 advances to 4,107 translated dispatches /
3,501 post-main dispatches. The old `INDIRECT_JUMP_MISS 0x80242D7C` is not
reproduced and the priority-24 TaskThread still enters its guest fiber.

This run does **not** yet prove queued `TaskThread::run` execution because the
diagnostics had no dedicated hit counter and `OSReceiveMessage` remains at 2.
The next diagnostic revision therefore records both `TaskThread::run hits` and
`GXSetProjection hits`.

The new exact blocker is:

```text
kind   : DIRECT
target : 0x8017301C
r1     : 0x80399008
r3     : 0x80399048
stage  : RMCP01_GX_DRAW_DONE
```

Pinned WiiCompiled maps `0x8017301C` to `GXSetProjection`. Its native
override converts the guest big-endian 4x4 matrix to host floats and forwards
the matrix plus projection type to Aurora GX.

The renderer remains ready with eight FIFO writes, zero display-list calls,
zero drawable FIFO work, zero `GXCopyDisp`, and zero presents. FST publication
remains valid and no DVD-read diagnostic is produced.

## Hardware result after #193 — GXSetViewport frontier

The first real-Switch run after #193 records:

```text
TaskThread::run hits : 1
GXSetProjection hits : 1
```

This removes the prior ambiguity: merged #192 and #193 are both hardware-PASS.
The scheduler remains recovered on the default/main thread and FST publication
remains valid.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x801733B4
r1     : 0x80399008
r3     : 0x80399048
stage  : RMCP01_GX_SET_PROJECTION
```

Pinned WiiCompiled maps `0x801733B4` to `GXSetViewport`. Its six scalar
float parameters use PPC `f1..f6` and are forwarded directly to Aurora GX.

Graphics state is otherwise unchanged: eight bootstrap FIFO writes, zero
display-list calls, no drawable FIFO work, zero `GXCopyDisp`, and zero
presents. No DVD-read status file is produced.

## Hardware result after #194 — GXSetScissor frontier

The first real-Switch run after #194 records:

```text
GXSetProjection hits : 1
GXSetViewport hits   : 1
```

This hardware-proves the merged viewport bridge. The scheduler remains recovered
on the default/main thread and FST publication remains valid.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x80173430
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_VIEWPORT
```

Pinned WiiCompiled maps `0x80173430` to `GXSetScissor`. Its native override
takes unsigned `r3..r6`, updates the guest GX scissor BP words at
`GXData+0x148/+0x14C`, clears `GXData+2`, then forwards the rectangle to
Aurora GX.

This specific run reports `TaskThread::run hits = 0`; it therefore does not
re-prove #192, but the priority-24 worker still reaches guest-fiber entry and
the previous hardware proof remains valid.

Graphics state is otherwise unchanged: eight bootstrap FIFO writes, zero
display-list calls, no drawable FIFO work, zero `GXCopyDisp`, and zero
presents. No DVD-read status file is produced.

## Hardware result after #195 — GXLoadPosMtxImm frontier

The first real-Switch run after #195 records:

```text
GXSetProjection hits : 1
GXSetViewport hits   : 1
GXSetScissor hits    : 1
```

This hardware-proves the merged scissor bridge. The scheduler remains recovered
on the default/main thread and FST publication remains valid.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x8017310C
r1     : 0x80399008
r3     : 0x80399018
stage  : RMCP01_GX_SET_SCISSOR
```

Pinned WiiCompiled maps `0x8017310C` to `GXLoadPosMtxImm`. Its native
override reads a guest 3x4 matrix from `r3`, converts twelve big-endian
float32 values to host order, and forwards the matrix plus `r4` matrix id to
Aurora GX.

This specific run again reports `TaskThread::run hits = 0`; the prior hardware
proof remains valid and the priority-24 worker still reaches guest-fiber entry.

Graphics state remains unchanged: eight bootstrap FIFO writes, zero display-
list calls, no drawable FIFO work, zero `GXCopyDisp`, and zero presents.
No DVD-read status file is produced.

## Hardware result after #196 — GXSetCurrentMtx frontier

The first real-Switch run after #196 records:

```text
TaskThread::run hits  : 1
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
```

This hardware-proves the merged position-matrix bridge and independently
re-proves the priority-24 TaskThread path. The scheduler remains recovered on
the default/main thread and FST publication remains valid.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x80173214
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_LOAD_POS_MTX_IMM
```

Pinned WiiCompiled maps `0x80173214` to `GXSetCurrentMtx`. Its native
override consumes only the matrix id from PPC `r3` and forwards it directly
to Aurora GX.

Graphics state remains unchanged: eight bootstrap FIFO writes, zero
display-list calls, no drawable FIFO work, zero `GXCopyDisp`, and zero
presents. No DVD-read status file is produced.

## Hardware result after #197 — GXClearVtxDesc frontier

The first real-Switch run after #197 records `GXSetCurrentMtx hits = 1` while
projection, viewport, scissor and position-matrix remain crossed. The durable
scheduler snapshot returns to default/main `0x80347498`. This run records
`TaskThread::run hits = 0`, but the priority-24 worker still reaches guest-
fiber entry and the prior TaskThread hardware proof remains valid.

The next exact blocker is PAL `GXClearVtxDesc (0x8016DC34)`. Pinned
WiiCompiled clears all 26 tracked vertex descriptors to `GX_NONE`,
conditionally invalidates the cached vertex-layout hash, preserves array
base/stride state, then calls Aurora `GXClearVtxDesc()`.

Graphics state remains unchanged: eight bootstrap FIFO writes, zero display-list
calls, no drawable FIFO work, zero `GXCopyDisp`, and zero presents. No DVD-read
status file is produced.

## Hardware result after #198 — GXSetVtxDesc frontier

The first real-Switch run after #198 records:

```text
TaskThread::run hits  : 1
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
GXSetCurrentMtx hits  : 1
GXClearVtxDesc hits   : 1
```

This hardware-proves the merged clear-descriptor bridge and independently
re-proves the priority-24 TaskThread path. The scheduler returns to default/main
`0x80347498`.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x8016D3A4
r3     : 0x00000009
stage  : RMCP01_GX_CLEAR_VTX_DESC
```

Pinned WiiCompiled maps `0x8016D3A4` to `GXSetVtxDesc`. The observed
attribute is `GX_VA_POS`; the descriptor type is not inferred because the
blocker diagnostic does not record `r4`.

The graphics report now also records `FIFO EVENT #9 ... value=0x48`. Pinned
Aurora emits that byte from `GXInvalidateVtxCache()`, making it the first
post-bootstrap GX/vertex-state FIFO traffic seen from RMCP01. It is not yet a
draw: display-list calls, drawable FIFO work, `GXCopyDisp`, and presents remain
zero. No DVD-read status file is produced.

## Hardware result after #199 — GXSetVtxAttrFmt frontier

The first real-Switch run after #199 records `TaskThread::run hits = 1` and
`GXSetVtxDesc hits = 1`, while the complete prior GX chain remains crossed.
The scheduler returns to default/main `0x80347498`.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x8016DC68
r3     : 0x00000000
stage  : RMCP01_GX_SET_VTX_DESC
```

Pinned WiiCompiled maps `0x8016DC68` to `GXSetVtxAttrFmt`. The observed
`r3 = 0` is `GX_VTXFMT0`; `r4..r7` are not inferred from the blocker
because they are not recorded there.

The rendered graphics report remains at nine FIFO writes, with the ninth
`0x48` byte still representing the first post-bootstrap GX/vertex-state
traffic. There is still no display-list call, drawable FIFO work,
`GXCopyDisp`, or present. No DVD-read status file is produced.

## Hardware result after #200 — GXSetNumChans frontier

The first real-Switch run after #200 records `GXSetVtxAttrFmt hits = 1` and
preserves the complete prior GX chain. The scheduler returns to default/main
`0x80347498`. This run has `TaskThread::run hits = 0`, but the priority-24
worker still reaches guest-fiber entry, so no TaskThread regression is inferred.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x8017054C
r3     : 0x00000001
stage  : RMCP01_GX_SET_VTX_ATTR_FMT
action : abort after durable blocker record
```

Pinned WiiCompiled maps `0x8017054C` to `GXSetNumChans` and implements it as
`GXSetNumChans((u8)n)`. The hardware value is `n = 1`.

The user's observed black screen followed by a return to hbmenu and a Switch
error message is consistent with the current first-blocker mechanism: after the
durable record is written, unsupported direct calls terminate through
`std::abort()`. No supplied `fast-track-exception.txt` indicates an
independent libnx exception for this run.

Graphics remain at nine FIFO writes, with no display list, drawable FIFO work,
`GXCopyDisp`, or present.

## Hardware result after #201 — GXSetChanMatColor frontier

The first real-Switch run after #201 records `TaskThread::run hits = 1`,
`GXSetNumChans hits = 1`, and the complete prior GX hit chain. The scheduler
returns to default/main `0x80347498`.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x80170474
r3     : 0x00000004
stage  : RMCP01_GX_SET_NUM_CHANS
```

Pinned WiiCompiled maps `0x80170474` to `GXSetChanMatColor`. It consumes
`r3 = channel` and `r4 = guest color pointer`, ensures the Aurora frame is
active, reads `Memory::Read32(r4)`, decodes the packed RGBA value, and forwards
it to Aurora `GXSetChanMatColor`.

Graphics remain at nine FIFO writes, with no display list, drawable FIFO work,
`GXCopyDisp`, or present. No DVD-read status file is produced.

## Build

The user's existing local RMCP01 translated product must already be present,
the same prerequisite as `build-local-fast-track.sh`.

```sh
git submodule update --init --recursive
MKW_JOBS=4 bash scripts/build-local-rendered-fast-track.sh
```

Output:

```text
WiiCompiled-Switch-local-rendered-fast-track.nro
```

This output contains local game-derived code and must not be uploaded or
committed.

## Hardware test

Copy the NRO into its own hbmenu directory and launch through title override /
application mode.

The graphics report is:

```text
/switch/WiiCompiled-Switch/rendered-fast-track-graphics.txt
```

Important milestones:

```text
STAGE CREATE_INSTANCE PASS
STAGE CREATE_SURFACE PASS
STAGE REQUEST_ADAPTER PASS
STAGE REQUEST_DEVICE PASS
STAGE CONFIGURE_SURFACE PASS
STAGE AURORA_GFX_INIT PASS
STAGE RENDERER_READY PASS
PASS FIRST_RMCP01_GX_PRESENT hadWork=1
```

The ordinary fast-track diagnostics remain authoritative for translated-runtime
progress:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-thread-events.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

A successful present with `hadWork=1` proves that real RMCP01 FIFO work reached
the native Switch graphics backend. Visual correctness is a separate question:
the first game-facing run may expose the next concrete GX, texture, DVD/FST or
resource-loading blocker.

## Hardware result after merged #203 — GXSetChanCtrl frontier

The real-Switch run on merged #203 records `GXSetChanMatColor hits = 1`,
`GXSetNumChans hits = 1`, `TaskThread::run hits = 1`, and scheduler
recovery to default/main `0x80347498`. The FST remains structurally valid at
`0x97DC0000`, size 64,224 bytes / 2,096 entries.

The renderer remains initialized and frame-active with nine FIFO writes, but
still has zero display-list calls, no FIFO-produced drawable work, zero
`GXCopyDisp` calls, and zero presents.

The next exact blocker is:

```text
kind   : DIRECT
target : 0x80170570
r1     : 0x80399008
r3     : 0x00000004
stage  : RMCP01_GX_SET_CHAN_MAT_COLOR
```

Pinned WiiCompiled maps `0x80170570` to `GXSetChanCtrl` and forwards
`r3..r9` as channel, enable, ambient/material sources, light mask, diffuse
function, and attenuation function. Only `r3 = 4` is present in the durable
blocker report; `r4..r9` are not guessed.

PR #204 now contains and has merged only the pinned `GXSetChanCtrl` boundary on `main` as `70805fb0b038ff447794fd18092a76a56dffbe46`. The next action is **not** another GX patch: build that exact revision with `scripts/build-local-rendered-fast-track.sh`, run it on real Switch hardware, and require durable progression beyond `0x80170570` before marking `GXSetChanCtrl` hardware-crossed. Do not pre-port the following texture/light/draw calls before hardware reaches them.

## Validation contract after the 2026-09-20 audit

The blocker-driven strategy remains the project default, with a stricter proof
standard documented in
[`FAST_TRACK_VALIDATION_POLICY.md`](FAST_TRACK_VALIDATION_POLICY.md).

For rendered RMCP01 changes, validation is now explicitly:

```text
exact hardware blocker
→ pinned WiiCompiled attribution
→ minimal boundary implementation
→ 5/5 Nintendo-data-free public CI on exact PR HEAD
→ successful private local-rendered-fast-track build
→ real-Switch run
→ durable progression beyond the tested target
```

The private rendered build is a required **sixth gate** because public CI cannot
contain the user-owned generated RMCP01 product and therefore does not exercise
the complete private Aurora/Dawn/NVK link graph.

A `GX... hits` counter alone is not enough to declare a boundary
hardware-crossed. The accepted proof is:

```text
hits > 0
AND blocker target != tested target
AND execution reaches a later durable dispatch / milestone
```

Every rendered hardware run must also compare scheduler coherence, FST
publication/validity, the previously crossed GX chain, FIFO writes,
display-list calls, drawable FIFO work, `GXCopyDisp`, present
success/failure, and any native exception against the previous accepted
baseline.

The latest run advances the FIFO from nine to eleven writes. With no proven
drawable work, display list, `GXCopyDisp`, or present, that still represents
GX-state setup rather than an RMCP01 draw/present milestone.

## Hardware result — 2026-09-21 GXSetNumTexGens frontier

The rendered real-Switch run progresses beyond `GXSetChanCtrl (0x80170570)`
and stops at the distinct DIRECT target `0x8016E5A4`, with `r3 = 0` and
stage `RMCP01_GX_SET_CHAN_CTRL`.

Pinned WiiCompiled maps `0x8016E5A4` to `GXSetNumTexGens` and consumes only
`r3`, narrowed to `u8`, before forwarding to Aurora.

The earlier periodic snapshot was written before this final transition. It
preserves the prior GX/FST/scheduler invariants and nine FIFO writes but still
has no display list, drawable FIFO work, `GXCopyDisp`, or present. The next
candidate implements only this exact texture-generator-count boundary.

## Hardware result — 2026-09-21 GXSetNumIndStages frontier

The rendered real-Switch run after merged #206 progresses beyond
`GXSetNumTexGens (0x8016E5A4)` and stops at the distinct DIRECT target
`0x80171B38`, with `r3 = 0` and stage
`RMCP01_GX_SET_NUM_TEX_GENS`.

Pinned WiiCompiled maps `0x80171B38` to `GXSetNumIndStages` and consumes
only `r3`, narrowed to `u8`, before forwarding to Aurora.

The earlier periodic snapshot predates the final transition. It preserves the
FST/renderer invariants and nine FIFO writes, but still has no display list,
drawable FIFO work, `GXCopyDisp`, or present. The next candidate implements
only this exact indirect-stage-count boundary.

## Hardware result — 2026-09-21 GXSetNumTevStages frontier

The rendered real-Switch run after merged #207 progresses beyond
`GXSetNumIndStages (0x80171B38)` and stops at the distinct DIRECT target
`0x801722A8`, with `r3 = 1` and stage
`RMCP01_GX_SET_NUM_IND_STAGES`.

Pinned WiiCompiled maps `0x801722A8` to `GXSetNumTevStages`, validates the
count against `GX_MAX_TEVSTAGE`, and for valid counts narrows `r3` to
`u8` before forwarding to Aurora.

The graphics log advances from nine to eleven FIFO writes, adding `0x61` and
`0x0F000000` state traffic. FST/renderer invariants remain intact, but there
is still no proven display list, drawable FIFO work, `GXCopyDisp`, or
present. The next candidate implements only this exact TEV-stage-count
boundary.

## Hardware result — 2026-09-21 GXSetTevOp frontier

The rendered real-Switch run after merged #208 progresses beyond
`GXSetNumTevStages (0x801722A8)` and stops at the distinct DIRECT target
`0x80171C4C`, with `r3 = 0` and stage
`RMCP01_GX_SET_NUM_TEV_STAGES`.

Pinned WiiCompiled maps `0x80171C4C` to `GXSetTevOp`, validates the TEV
stage with `stage < GX_MAX_TEVSTAGE`, and forwards `r3/r4` as
`GXTevStageID/GXTevMode` to Aurora. The blocker diagnostic did not capture
`r4`, so no hardware mode value is asserted; the candidate consumes the live
guest register directly.

The graphics log remains at eleven FIFO writes. FST/renderer invariants remain
intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetTevOp` boundary.

## Hardware result — 2026-09-21 GXSetTevOrder frontier

The rendered real-Switch run after merged #209 progresses beyond
`GXSetTevOp (0x80171C4C)` and stops at the distinct DIRECT target
`0x8017214C`, with `r3 = 0` and stage
`RMCP01_GX_SET_TEV_OP`.

Pinned WiiCompiled maps `0x8017214C` to `GXSetTevOrder`, validates the TEV
stage with `stage < GX_MAX_TEVSTAGE`, and forwards `r3-r6` as
`GXTevStageID/GXTexCoordID/GXTexMapID/GXChannelID` to Aurora. The blocker
diagnostic did not capture `r4-r6`, so no hardware texcoord, texmap, or
channel values are asserted; the candidate consumes the live guest registers
directly.

The graphics log remains at eleven FIFO writes. FST/renderer invariants remain
intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetTevOrder` boundary.

## Hardware result — 2026-09-21 GXSetBlendMode frontier

The rendered real-Switch run after merged #210 progresses beyond
`GXSetTevOrder (0x8017214C)` and stops at the distinct DIRECT target
`0x8017277C`, with `r3 = 0` and stage
`RMCP01_GX_SET_TEV_ORDER`.

Pinned WiiCompiled maps `0x8017277C` to `GXSetBlendMode` and forwards
`r3-r6` as `GXBlendMode/GXBlendFactor/GXBlendFactor/GXLogicOp` to Aurora
without additional validation. The blocker diagnostic did not capture
`r4-r6`, so no hardware factor or logic-op values are asserted; the candidate
consumes the live guest registers directly.

The graphics log remains at eleven FIFO writes. FST/renderer invariants remain
intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetBlendMode` boundary.

## Hardware result — 2026-09-21 GXSetColorUpdate frontier

The rendered real-Switch run after merged #211 progresses beyond
`GXSetBlendMode (0x8017277C)` and stops at the distinct DIRECT target
`0x801727CC`, with `r3 = 1` and stage
`RMCP01_GX_SET_BLEND_MODE`.

Pinned WiiCompiled maps `0x801727CC` to `GXSetColorUpdate` and forwards
`r3` directly as `GXBool` to Aurora without additional validation.

The graphics log remains at eleven FIFO writes. FST/renderer invariants remain
intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present.

PR #212 now implements only this exact `GXSetColorUpdate` boundary and is
squash-merged on `main` as
`8aea70d0a3a8378311428eb5420760e4f763a40d` after 5/5 public CI. The next
step is the private rendered build of that exact revision and real-Switch
validation requiring durable progression beyond `0x801727CC`. No following
pixel/TEV/texture/draw/resource boundary is selected before that hardware run.

## Hardware result — 2026-09-21 GXSetAlphaUpdate frontier

The rendered real-Switch run built from merged #212
(`8aea70d0a3a8378311428eb5420760e4f763a40d`) progresses beyond
`GXSetColorUpdate (0x801727CC)` and stops at the distinct DIRECT target
`0x801727F8`, with `r3 = 1` and stage
`RMCP01_GX_SET_COLOR_UPDATE`.

Pinned WiiCompiled maps `0x801727F8` to `GXSetAlphaUpdate` and forwards
`r3` directly as `GXBool` to Aurora without additional validation.

The graphics log still reaches eleven FIFO writes. FST/renderer invariants
remain intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetAlphaUpdate` boundary.

## Hardware result — 2026-09-21 GXSetZMode frontier

The latest rendered RMCP01 hardware run progresses beyond
`GXSetAlphaUpdate (0x801727F8)` and stops at the distinct DIRECT target
`0x80172824`, with `r3 = 0` and stage
`RMCP01_GX_SET_ALPHA_UPDATE`.

Pinned WiiCompiled maps `0x80172824` to `GXSetZMode`, consuming live
`r3/r4/r5` as compare-enable / `GXCompare` / update-enable. The current
blocker did not record `r4/r5`; the candidate therefore reads them live rather
than fabricating values, and expands the next durable blocker record to capture
them.

The graphics log still reaches eleven FIFO writes. FST/renderer invariants
remain intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetZMode` boundary.

## Hardware result — 2026-09-21 GXSetCullMode frontier

The latest rendered RMCP01 hardware run progresses beyond
`GXSetZMode (0x80172824)` and stops at the distinct DIRECT target
`0x8016F3B8`, with `r3 = 2` and stage
`RMCP01_GX_SET_Z_MODE`.

Pinned WiiCompiled maps `0x8016F3B8` to `GXSetCullMode`, consuming only
live `r3` and forwarding it directly as `GXCullMode` to Aurora.

The graphics log still reaches eleven FIFO writes. FST/renderer invariants
remain intact, but there is still no proven display list, drawable FIFO work,
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetCullMode` boundary.

## Hardware result — 2026-09-21 GXBegin frontier

The latest rendered RMCP01 hardware run progresses beyond
`GXSetCullMode (0x8016F3B8)` and stops at the distinct DIRECT target
`GXBegin (0x8016F0F0)`, with `r3/r4/r5 = 0x80 / 0 / 4` and stage
`RMCP01_GX_SET_CULL_MODE`.

This is the first observed game-facing primitive boundary. Pinned WiiCompiled
republishes the tracked VCD/VAT state, initializes `g_hleGxState` for the
begin packet, and leaves subsequent real vertex payload to the pinned
`HleFifoWrite` decoder that is already wired to Aurora/Dawn/NVK in this
rendered target.

The run exposing the blocker still reports eleven FIFO state writes and does
not yet prove drawable FIFO work, `GXCopyDisp`, or present because the
`GXBegin` bridge had not executed yet. The next hardware run is therefore the
first one that can legitimately flip `FIFO produced work` to YES.

## Hardware result — 2026-09-21 first RMCP01 FIFO work / endRender frontier

The latest rendered RMCP01 run crosses `GXBegin (0x8016F0F0)` and emits
`PASS FIRST_RMCP01_FIFO_WORK`. Real game vertex payload is therefore
reaching the pinned `HleFifoWrite` decoder and producing Aurora frame work.

The new durable blocker is `INDIRECT_CALL_MISS 0x8020FF9C` with stage
`RMCP01_FIFO_RENDER_WORK`. Pinned WiiCompiled maps that address to
`EGG::AsyncDisplay::endRender`, which dispatches
`EGG::Display::copyEFBtoXFB (0x80219FB4)` followed by
`GXSetDrawDoneCallback (0x8016ED50)`.

An earlier 2,393-dispatch periodic snapshot still reports zero GXBegin hits,
nine FIFO writes and no work; it predates the later final transition and does
not contradict the rendered work marker.

This closes the "first drawable RMCP01 work" milestone. The next hardware
question is whether the real endRender/copy path reaches the already prepared
`GXCopyDisp` present seam. The candidate resolves only endRender and leaves
both nested guest calls to the normal translated dispatcher.

## Hardware result — 2026-09-21 GXSetCopyFilter frontier

The latest rendered hardware run preserves the first real RMCP01 drawable
work, hardware-crosses `EGG::AsyncDisplay::endRender (0x8020FF9C)`, and
stops at the distinct DIRECT target `GXSetCopyFilter (0x8016FA40)`.

The durable snapshot records `GXBegin hits = 1`,
`AsyncDisplay endRender = 1`, 23 FIFO writes, and
`FIFO produced work = YES`; `GXCopyDisp` and present remain zero.

Pinned WiiCompiled maps `0x8016FA40` to `GXSetCopyFilter`, consuming
live r3..r6 and copying 24-byte sample-pattern plus 7-byte vertical-filter
data from guest RAM. The hardware blocker captured r3/r4/r5 but not r6, so
the candidate reads r6 live and extends the blocker format rather than
inventing it.

This places the hardware frontier inside the EFB/XFB copy-configuration path,
but the exact following copy/present boundary remains hardware-defined.

## Hardware result — 2026-09-21 first RMCP01 GPU present / GXFlush frontier

The rendered RMCP01 path now closes the first game-facing presentation gate.

Renderer evidence:

```text
PASS FIRST_RMCP01_FIFO_WORK
...
PASS FIRST_RMCP01_GX_PRESENT hadWork=1
```

The PASS-present record is emitted only after `g_surface.Present()` succeeds.
`hadWork=1` confirms that the frame already contained real RMCP01 FIFO/Aurora
work.

The final durable blocker is then:

```text
DIRECT 0x8016E654
stage = RMCP01_GX_PRESENTED
```

Pinned WiiCompiled maps `0x8016E654` to no-argument `GXFlush()`.

This proves the first real RMCP01 GPU present through
`HleFifoWrite → Aurora GX → Dawn/WebGPU → Vulkan/NVK → NWindow`.
It does not by itself prove that the pixels are already a visually correct
Mario Kart Wii image, so visual confirmation remains a separate milestone.

## Hardware result — 2026-09-22 GXFlush crossed / TaskThread dispatch diagnostic frontier

The next rendered hardware run crosses `GXFlush (0x8016E654)` repeatedly and
continues durably beyond it:

```text
GXFlush hits          : 23
GXCopyDisp calls      : 23
present successes     : 23
present failures      : 0
FIFO produced work    : YES
```

The renderer still records `PASS FIRST_RMCP01_GX_PRESENT hadWork=1`, the FST
remains valid at `0x97DC0000`, and the watchdog remains ACTIVE. `GXFlush`
is therefore hardware-crossed.

The later durable blocker is:

```text
INDIRECT_CALL_MISS 0x8042E458
guest pc = 0x8024373C
r1       = 0x8042E458
r3       = 0x80210078
r4       = 0x8042E438
r5       = 1
stage    = HOST_CONTEXT_SWITCH_RETURNED
```

Thread telemetry identifies `0x8042E480` as the priority-24
`EGG::TaskThread` worker with `stored_r1=0x8042E458` and virtual
`run()=0x80242D7C`. The miss target is therefore the worker's guest stack
pointer, not a function address.

Pinned `TaskThread_run_HLE_80242d7c` uses `r1 - 0x20` as the blocking
`OSReceiveMessage` output slot, then reads `job/callback/arg/onDone` and
dispatches the callback indirectly. The observed `r4 = r1 - 0x20` and
`r5 = 1` match that path, but the current logs do not capture the job record
itself. The next candidate therefore adds only
`fast-track-task-thread-last-dispatch.txt` telemetry immediately before those
indirect dispatches. No scheduler, HostContext, resource, DVD or GX behavior is
changed until hardware identifies which job field produced the target.

## Hardware result — 2026-09-22 TaskThread stack-job attribution

The follow-up rendered run preserves the first RMCP01 present and repeated GX
path, while the dedicated TaskThread diagnostic captures:

```text
job      = 0x8042E448
callback = 0x8042E458
arg      = 0x80210078
token    = 0
onDone   = 0x801AA0F0
r1       = 0x8042E458
```

RMCP01's EGG implementation sends a pointer to a heap-allocated `TJob` slot
through the inherited message queue. The observed job instead aliases the
worker stack and decodes its stack pointer as the callback. This excludes a
missing callback translation as the immediate cause.

The next candidate is diagnostics-only: extend the TaskThread record with its
queue/buffer/job-array fields and capture each rendered `OSSendMessage`
queue/message pair. Hardware must establish whether the producer sends the bad
stack pointer or the queue storage returns it before any behavioral fix.

## Hardware result — 2026-09-22 valid TaskThread send / receive-slot frontier

The next rendered run resolves the producer side:

```text
TaskThread::request send:
queue       = 0x8042BBFC
msg         = 0x8042E7DC
array       = 0x8042E7A8
count       = 5
first       = 0
used_before = 0

later TaskThread dispatch:
queue first = 1
queue used  = 0
jobs        = 0x8042E7DC
job count   = 5
out slot    = 0x8042E438
job read    = 0x8042E448
callback    = 0x8042E458
```

The producer therefore supplies the correct first `mJobs[]` pointer and the
queue consumes one element. The invalid value appears at the blocking receive
output slot after that valid send. Its stack-shaped chain is consistent with a
guest frame overwrite, but the exact clobbering phase is not yet proven.

The worker metadata is `mJobCount=5`, `mStackSize=0x2800`; this supersedes
the earlier ResourceManager attribution for this exact object.

The next candidate adds diagnostics only around each
`OSReceiveMessage` phase: dequeue, output write, wakeup-senders, interrupt
restore and blocking-sleep return.

## Hardware result — 2026-09-23 masked VI poll correction frontier

The phase-level hardware trace now closes the previous receive-slot attribution:

```text
after-dequeue:
  msg       = 0x8042E7DC
  out_value = 0x8042E448

after-output-write:
  msg       = 0x8042E7DC
  out_value = 0x8042E7DC

after-wakeup-senders:
  msg       = 0x8042E7DC
  out_value = 0x8042E448
```

The sender wait queue is empty (`send_head=0`, `send_tail=0`) on this call,
so the native wakeup body has no runnable sender to process. The Switch
`InvokeDirectCpu` path nevertheless executes the time-driven VI poll before
the native call. `OSReceiveMessage` has guest interrupts disabled throughout
this critical section.

The current correction therefore changes only the Switch VI service policy:
`mkw_switch_hle_vi_poll_retrace` returns while guest interrupts are disabled.
This matches the pinned runtime's interrupt-aware deferred callback policy and
avoids injecting retrace callbacks into an interrupt-masked guest critical
section.

Hardware acceptance is that `0x8042E7DC` remains in the output slot through
`after-wakeup-senders` and that the next durable blocker moves beyond the
stack-pointer callback `0x8042E458`.

## Hardware result — 2026-09-23 first DVD read / idle scheduler frontier

The next rendered run validates the masked-VI-poll correction:

```text
after-output-write     = 0x8042E7DC
after-wakeup-senders   = 0x8042E7DC
after-restore-interrupts = 0x8042E7DC
```

TaskThread therefore dispatches the real job:

```text
job      = 0x8042E7DC
callback = 0x8000B53C
arg      = 0
onDone   = 0
```

That callback reaches the local DVD bridge and produces:

```text
status = read-pass
path   = /Boot/Strap/eu/English.szs
size   = 299969
result = 299969
```

The new blocker is `SELECTTHREAD_IDLE_POLL` at PAL
`SelectThread (0x801A9C08)` after the priority-24 TaskThread loops back into
a blocking receive and no runnable thread remains. The default thread is
already WAITING on queue `0x804294A4`.

The current candidate adds diagnostics only to `OSSleepThread` and the idle
frontier to identify who parked that default thread and which pinned wake
source is actually required. No idle-loop subsystem is enabled yet.

This run aborts before the previously hardware-proven drawable/present path;
that earlier GPU proof remains valid but is not re-exercised by this shorter
resource/scheduler run.

## Hardware result — 2026-09-23 AsyncDisplay VI idle wake attribution

The new scheduler telemetry identifies the default-thread wait exactly:

```text
thread   = 0x80347498
queue    = 0x804294A4
priority = 16
lr       = 0x8020FE50
```

The same run records the active `EGG::AsyncDisplay` object at
`0x8042944C`. RMCP01's class layout places its thread/sync queue at
`+0x58`, so:

```text
0x8042944C + 0x58 = 0x804294A4
```

The EGG reference implementation used by the RMCP01 header source has
`syncTick()` call `OSSleepThread(&mSyncQueue)` and `postVRetrace()`
call `OSWakeupThread(&mSyncQueue)`. The required idle wake source is
therefore VI/post-retrace.

Pinned WiiCompiled's `SelectThread` already polls VI and waits toward the
next retrace deadline when no guest thread is runnable. The Switch candidate
ports only that VI slice and mirrors the pin's
`VI_HLE_IsAdvancingRetrace()` guard so `OSWakeupThread` marks the awakened
thread runnable without recursively rescheduling from inside the retrace
callback.

Timers, alarms and audio idle pumps remain unimplemented until hardware asks
for them.

## Hardware result — 2026-09-24 decodeSZS frontier

The SelectThread VI idle wake is now hardware-validated:

```text
scheduler_pending = 0x02008000
default_state     = READY
default_queue     = 0x80347830
```

The default thread later runs again with no wait queue, and the rendered path
recovers `FIRST_RMCP01_FIFO_WORK`, `GXCopyDisp=1`, one successful present
and zero present failures.

The next durable blocker is:

```text
DIRECT 0x80218C2C
r3 = 0x94226C20
r4 = 0x80F10300
stage = RMCP01_GX_FLUSH
```

Pinned WiiCompiled maps `0x80218C2C` exactly to
`EGG::Decomp::decodeSZS`. The source pointer equals the successful
`/Boot/Strap/eu/English.szs` DVD buffer, so the candidate ports only this
exact native Yaz0 decoder boundary. Neighboring ASH/ASR/resource functions
remain untouched until hardware reaches them.

## Hardware result — 2026-09-24 SZS crossed / GXInitTexObj frontier

The rendered run hardware-validates the pinned SZS decoder:

```text
decode-pass
299969 compressed bytes consumed
2627200 decompressed bytes produced
```

The existing TaskThread, DVD and VI-idle invariants remain healthy, and the
graphics report again records both `FIRST_RMCP01_FIFO_WORK` and
`FIRST_RMCP01_GX_PRESENT hadWork=1`.

The new durable blocker is:

```text
DIRECT 0x801707F8
r3 = 0x901136B4
r4 = 0x80F103E0
r5 = 0x00000340
r6 = 0x000001C8
```

RMCP01 maps this exactly to `GXInitTexObj`. The image data begins only
`0xE0` bytes into the decompressed `English.szs` output. The blocker does
not capture r7-r10, so the candidate consumes the live guest registers for
format/wrapS/wrapT/mipmap and mirrors only the pinned `GXInitTexObj`
contract. Texture load/LOD/CI/TLUT neighbors remain unported until hardware
reaches them.

## Hardware result — 2026-09-24 GXInitTexObj crossed / IOS_Open frontier

The rendered run crosses `GXInitTexObj (0x801707F8)` and preserves the
resource/render invariants, including real RMCP01 FIFO work, one
`GXCopyDisp`, one successful present and zero present failures.

The new blocker is:

```text
DIRECT 0x801938F8
r3 = 0x802A2160
r4 = 0
r5 = 0x803990A0
r6 = 0
stage = HOST_CONTEXT_SWITCH_RETURNED
```

Pinned WiiCompiled maps this exactly to `NAND_IOS_Open_HLE(pathPtr, mode)`.
The current durable record does not include the C string pointed to by
`r3`. Since pinned behavior differs for IOS devices versus NAND files, the
candidate adds diagnostics only for the exact path and mode. No IOS/NAND/ISFS
behavior is added before hardware identifies that path.

## Hardware result — 2026-09-24 IOS_Open KD request frontier

The diagnostic run identifies the exact `IOS_Open (0x801938F8)` request as
`/dev/net/kd/request`, mode 0. Pinned WiiCompiled classifies this as
`DeviceKind::KdRequest` and allocates network-device handles from fd 2000
when networking is enabled, which is the pinned default.

The same run records `GXInitTexObj status=init-pass` for the 832x456 texture,
so that graphics boundary is hardware-crossed.

PR #232 now mirrors only this exact KD-request open and is squash-merged on
`main` as `4f1d0188c61d0e12267e5468c1b6591df1d29a4d` after all five required
public CI workflows passed. It deliberately leaves IOS ioctl/ioctlv/close, KD
commands, NCD, IP, SSL, DNS and sockets unsupported until hardware reaches
them.

The next real-Switch acceptance gate is:

```text
fast-track-ios-open-kd-request.txt:
  status=open-pass
  path=/dev/net/kd/request
  mode=0
  fd=2000
```

Execution must then durably progress beyond `0x801938F8` while preserving the
already proven DVD/SZS/scheduler/render invariants. The next distinct hardware
blocker, not a predicted neighbor, becomes the following frontier.

## Hardware result — 2026-09-24 KD open crossed / IOS_Ioctl cmd 2 frontier

The real-Switch run validates the merged #232 bridge:

```text
status=open-pass
path=/dev/net/kd/request
mode=0
fd=2000
```

Execution then reaches the distinct blocker. The diagnostic revision now
captures the complete call:

```text
DIRECT 0x80194290
r3 = 0x000007D0
r4 = 0x00000002
r5 = 0x80356F20
r6 = 0x00000020
r7 = 0x80356F40
r8 = 0x00000020
```

Pinned WiiCompiled maps this to `NAND_IOS_Ioctl_Entry_HLE`; fd 2000 selects
the KD request device and command 2 is the NWC24 try-suspend-scheduler probe.
In the initial Boot phase, `HandleKdIoctl` writes `-42` to the result word at
`outBuf` and returns IOS result 0.

The candidate mirrors only that first hardware-observed probe. It validates
fd 2000 / command 2 / 0x20-byte input and output buffers, writes `-42` through
the live output pointer, returns 0 in r3, and marks that Boot probe consumed.
A repeated command 2, command 1/3, ioctlv, sockets, DNS, NCD, IP, SSL and other
network services remain unsupported until a later hardware blocker proves
them.


## Hardware result — 2026-09-25 KD cmd2 crossed / IOS_Close frontier

The real-Switch run built from merged PR #235 proves durable progression beyond
the first KD command-2 Boot probe:

```text
fast-track-ios-ioctl-kd-cmd2.txt
status=cmd2-boot-probe-pass
fd=2000
cmd=2
in=0x80356F20/0x20
out=0x80356F40/0x20
```

The established resource/scheduler/render path is preserved: FST remains
published at 64,224 bytes / 2,096 entries; `English.szs` remains read-pass at
299,969 bytes and decode-pass at 2,627,200 bytes; `GXInitTexObj` remains
832x456 format 4; TaskThread/scheduler activity remains healthy; real RMCP01
FIFO work and one successful `GXCopyDisp` presentation are still observed.

The later distinct blocker is:

```text
DIRECT 0x80193AD8
r3 = 0x000007D0
stage = HOST_CONTEXT_SWITCH_RETURNED
```

Pinned WiiCompiled maps `0x80193AD8` exactly to
`NAND_IOS_Close_HLE(fd)`. fd 2000 is the same first KD request handle. For a
network fd, pinned behavior calls `Network_HLE_Close`, removes that device
from the network handle map, and returns 0.

The candidate mirrors only this exact close. It tracks only the proven first KD
handle, retires it on fd 2000, returns IOS result 0 in r3, and aborts on any
other close as a fresh hardware-defined frontier. It does not add another KD
command, ioctlv, NCD, IP, SSL, DNS, socket, or generic IOS-close support.


## Hardware result — 2026-09-25 IOS_Close crossed / GXLoadTexObj frontier

The real-Switch run built from merged PR #236 records:

```text
fast-track-ios-close-kd-request.txt
status=close-pass
fd=2000
```

That status is followed by durable translated progress, so `IOS_Close
(0x80193AD8)` is hardware-crossed rather than merely hit.

The same run advances materially deeper into game startup:

```text
RKSystem::run hits = 1
TaskThread::run hits = 2
DVD read: /rel/StaticR.rel
result = 4903876
```

The established renderer path is also preserved with 29 RMCP01 FIFO writes,
real FIFO work, one `GXCopyDisp`, one successful present, and zero failures.

The new distinct blocker is:

```text
DIRECT 0x80170F2C
r3 = 0x901136B4
r4 = 0x00000000
stage = RMCP01_GX_SET_CHAN_CTRL
```

Pinned WiiCompiled maps this exactly to `GXLoadTexObj(oa, tid)`.

The live `oa=0x901136B4` is not the previously captured init-pass object
`0x901136D4`; it is a separate 32-byte guest GXTexObj. Pinned
`TryGetOrExtractTexObjMeta` may decode dimensions, backing address, format,
wrap, mipmap and LOD state directly from those guest bytes. The correct load
path can also diverge for CI/TLUT or unsupported formats.

Therefore the current candidate is diagnostics-only. It extends the durable
blocker record for target `0x80170F2C` with all eight 32-bit guest words plus
the directly decoded data address, width/height, word5/word2 format values,
wrap S/T and mipmap flag. No GXLoadTexObj/Aurora behavior is added yet.


## Hardware result — 2026-09-25 GXLoadTexObj descriptor captured

The diagnostics-only run reproduces the same blocker and captures the complete
guest GXTexObj at `0x901136B4`:

```text
oa  = 0x901136B4
tid = 0
word0 = 0x00000090
word1 = 0x00000000
word2 = 0x00471F3F
word3 = 0x0007881F
word4 = 0x00000000
word5 = 0x00000004
word6 = 0x00000000
word7 = 0x5CA00202
```

Pinned guest decoding yields:

```text
data   = 0x00F103E0
width  = 832
height = 456
format = 4 (word5 and word2 agree)
wrap   = clamp / clamp
mipmap = false
```

Format 4 uses 4x4 RGBA8 blocks. At 832x456 this is 208x114 = 23,712
blocks, matching the descriptor's block count. At 64 bytes per block the exact
payload is `0x172800` bytes.

This is a non-CI descriptor; no TLUT behavior is required by the observed call.
The candidate therefore accepts only the exact eight hardware-captured words,
requires the full payload to be mapped, reconstructs an Aurora GXTexObj from
the live guest backing, applies the decoded linear/linear zero-LOD state, binds
only map 0, then mirrors the pinned GXData dirty/writeback side effects.

Any different object address, map id, descriptor word, format, dimensions,
wrap, mipmap state or backing becomes a fresh `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`
frontier. No neighboring CI/TLUT/LOD/invalidation API is pre-ported.
