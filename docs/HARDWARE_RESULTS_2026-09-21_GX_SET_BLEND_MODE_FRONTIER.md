# Hardware result — GXSetBlendMode frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch run of the rendered RMCP01 fast-track built from merged #210
progresses beyond PAL `GXSetTevOrder (0x8017214C)` and exposes a new exact
unsupported DIRECT dispatch.

The final durable blocker is:

```text
kind   : DIRECT
target : 0x8017277c
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_TEV_ORDER
action : abort after durable blocker record
```

The stage is set inside the merged GXSetTevOrder bridge, so reaching the
distinct later target proves durable progression beyond `0x8017214C`.

The periodic last-dispatch snapshot predates the final transition. It still
records `GXSetTevOrder hits = 0`; that timing fact does not override the later
blocker record.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that revision PAL `0x8017277C` is `GXSetBlendMode`:

```cpp
extern "C" void GX__SetBlendMode_8017277c(
    uint32_t t, uint32_t s, uint32_t d, uint32_t op) {
    GXSetBlendMode(
        static_cast<GXBlendMode>(t),
        static_cast<GXBlendFactor>(s),
        static_cast<GXBlendFactor>(d),
        static_cast<GXLogicOp>(op));
}
```

There is no additional validation or guest-memory access at this pinned
boundary.

The current blocker diagnostic captured only `r3 = 0`, corresponding to the
blend-mode type. It did not record `r4-r6`, so source factor, destination
factor, and logic-op values are not known from this run and must not be
invented. The bridge reads live guest `r4-r6` from the CPU context.

## Narrow Switch implementation

The candidate bridge:

- reads `r3` as blend-mode type;
- reads live `r4` as source blend factor;
- reads live `r5` as destination blend factor;
- reads live `r6` as logic op;
- records `RMCP01_GX_SET_BLEND_MODE`;
- performs no validation beyond the pinned enum casts;
- calls Aurora `GXSetBlendMode` with the exact four enum conversions in the
  rendered fast-track;
- leaves synthetic/headless builds side-effect-free;
- exposes only PAL `0x8017277C`;
- adds `GXSetBlendMode hits` durable telemetry.

The synthetic probe uses zero values for `r3-r6` solely as
Nintendo-data-free compile/link coverage. Only hardware `r3 = 0` is claimed
from this run.

No neighboring pixel, blend, Z, TEV, texture, draw, DVD, or resource boundary
is pre-ported.

## Graphics state

The rendered graphics log remains at eleven FIFO writes. The final events remain
GX state traffic, not drawable work. The periodic snapshot records no
display-list call, no FIFO-produced drawable work, no `GXCopyDisp`, and no
present.

## Preserved invariants

FST remains published at `0x97DC0000`, size 64,224 bytes / 2,096 entries.
The renderer remains initialized/frame-active and the translated watchdog stays
ACTIVE before the final blocker.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetBlendMode hits` must become non-zero;
2. `0x8017277C` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware, not from adjacent pixel-state
symbols.
