# Hardware result — GXSetColorUpdate frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch run of the rendered RMCP01 fast-track built from merged #211
progresses beyond PAL `GXSetBlendMode (0x8017277C)` and exposes a new exact
unsupported DIRECT dispatch:

```text
kind   : DIRECT
target : 0x801727cc
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_BLEND_MODE
action : abort after durable blocker record
```

The stage is set inside the merged GXSetBlendMode bridge, so reaching the
distinct later target proves durable progression beyond `0x8017277C`.

The periodic last-dispatch snapshot predates the final transition; its zero
GXSetBlendMode/GXSetColorUpdate hit counts do not override the later final
blocker record.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x801727CC` is:

```cpp
extern "C" void GX__SetColorUpdate_801727cc(uint32_t en) {
    GXSetColorUpdate(static_cast<GXBool>(en));
}
```

The hardware captured `r3 = 1`. There is no additional validation, memory
access, or hidden argument at this pinned boundary.

## Narrow Switch implementation

The candidate bridge reads live `r3`, records
`RMCP01_GX_SET_COLOR_UPDATE`, and in the rendered fast-track forwards the
value exactly as `static_cast<GXBool>(r3)` to Aurora `GXSetColorUpdate`.
Synthetic/headless builds remain side-effect-free.

A Nintendo-data-free synthetic probe uses `r3 = 1`, matching the captured
hardware scalar.

No neighboring `GXSetAlphaUpdate`, `GXSetZMode`, pixel-format, TEV,
texture, draw, DVD, or resource boundary is pre-ported.

## Preserved invariants

The rendered graphics log still reaches eleven FIFO writes. There is still no
proven display-list call, FIFO-produced drawable work, `GXCopyDisp`, or
present. FST publication remains 64,224 bytes / 2,096 entries, and the
renderer remains initialized/frame-active.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetColorUpdate hits` must become non-zero;
2. `0x801727CC` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware.
