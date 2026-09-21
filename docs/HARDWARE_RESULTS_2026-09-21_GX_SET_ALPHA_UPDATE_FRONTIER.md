# Hardware result — GXSetAlphaUpdate frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch rendered RMCP01 run built from merged #212
(`8aea70d0a3a8378311428eb5420760e4f763a40d`) progresses beyond
PAL `GXSetColorUpdate (0x801727CC)` and exposes the next exact unsupported
DIRECT dispatch:

```text
kind   : DIRECT
target : 0x801727f8
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_COLOR_UPDATE
action : abort after durable blocker record
```

The stage is set inside the merged GXSetColorUpdate bridge, so reaching the
distinct later target proves durable progression beyond `0x801727CC`.

The periodic durable snapshot predates the final transition. Its zero
GXSetColorUpdate/GXSetAlphaUpdate hit counters therefore do not override the
later blocker record.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x801727F8` is:

```cpp
extern "C" void GX__SetAlphaUpdate_801727f8(uint32_t en) {
    GXSetAlphaUpdate(static_cast<GXBool>(en));
}
```

Hardware captured `r3 = 1`. There is no additional validation, guest-memory
read, or hidden argument at this boundary.

## Narrow Switch implementation

The candidate bridge reads live `r3`, records
`RMCP01_GX_SET_ALPHA_UPDATE`, and in the rendered fast-track forwards the
value exactly as `static_cast<GXBool>(r3)` to Aurora `GXSetAlphaUpdate`.
Synthetic/headless builds remain side-effect-free.

A Nintendo-data-free synthetic probe uses `r3 = 1`, matching the captured
hardware scalar.

No neighboring `GXSetZMode`, `GXSetZCompLoc`, pixel-format, dither,
destination-alpha, TEV, texture, draw, DVD, or resource boundary is pre-ported.

## Preserved invariants

- PAL main remains reached.
- FST remains published at `0x97DC0000`, 64,224 bytes / 2,096 entries.
- renderer remains initialized with an active frame.
- independent watchdog remains ACTIVE.
- rendered graphics log reaches eleven FIFO writes.
- there is still no proven display-list call, FIFO-produced drawable work,
  `GXCopyDisp`, or successful present.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetAlphaUpdate hits` must become non-zero;
2. `0x801727F8` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware.
