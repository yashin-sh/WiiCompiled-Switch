# M3 — local RMCP01 rendered fast-track

Tracking: #117, #162, #154, #4

Status: **renderer and local FST publication hardware-proven; #186 identified later priority-6 OSThread `0x90112660`; the first #188 hardware run exposed an earlier VI-poll register-clobber regression at the initial guest-fiber entry, now fixed in code pending hardware revalidation**.

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
