# M3 — local RMCP01 rendered fast-track

Tracking: #117, #162, #154, #4

Status: **renderer and local FST publication hardware-proven; #201 hardware-proves `GXSetNumChans` and re-proves `TaskThread::run`; the current exact blocker is PAL `GXSetChanMatColor` at `0x80170474`**.

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
`GXCopyDisp`, or present. The next candidate implements only this exact
`GXSetColorUpdate` boundary.
