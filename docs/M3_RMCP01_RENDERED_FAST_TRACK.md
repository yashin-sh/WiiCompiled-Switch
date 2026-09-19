# M3 — local RMCP01 rendered fast-track

Tracking: #117, #162, #154, #4

Status: **renderer hardware-proven; local DVD/FST publication is the current real-Switch gate**.

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
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

A successful present with `hadWork=1` proves that real RMCP01 FIFO work reached
the native Switch graphics backend. Visual correctness is a separate question:
the first game-facing run may expose the next concrete GX, texture, DVD/FST or
resource-loading blocker.
