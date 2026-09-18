# M3 — local RMCP01 rendered fast-track

Tracking: #117, #162, #4

Status: **implemented; real-Switch hardware test next**.

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
