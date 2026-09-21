# Hardware result — GXSetTevOrder frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch run of the rendered RMCP01 fast-track built from merged #209
progresses beyond PAL `GXSetTevOp (0x80171C4C)` and exposes a new exact
unsupported DIRECT dispatch.

The final durable blocker is:

```text
kind   : DIRECT
target : 0x8017214c
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_TEV_OP
action : abort after durable blocker record
```

The stage is set inside the merged GXSetTevOp bridge, so reaching the distinct
later target proves durable progression beyond `0x80171C4C`.

The periodic last-dispatch snapshot predates the final transition. It still
records `GXSetTevOp hits = 0`; that timing fact does not override the later
blocker record.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that revision PAL `0x8017214C` is `GXSetTevOrder`:

```cpp
extern "C" void GX__SetTevOrder_8017214c(
    uint32_t s, uint32_t c, uint32_t m, uint32_t col) {
    if (!TevStageOk(s)) return;
    GXSetTevOrder(
        (GXTevStageID)s,
        (GXTexCoordID)c,
        (GXTexMapID)m,
        (GXChannelID)col);
}
```

`TevStageOk(s)` accepts only `s < GX_MAX_TEVSTAGE`.

The current blocker diagnostic captured `r3 = 0`, corresponding to the TEV
stage. It did not record `r4-r6`, so texcoord, texmap, and channel values are
not known from this run and must not be invented. The bridge reads live guest
`r4-r6` from the CPU context and forwards them exactly as pinned semantics
require.

## Narrow Switch implementation

The candidate bridge:

- reads `r3` as the TEV stage;
- reads live `r4` as the texture-coordinate id;
- reads live `r5` as the texture-map id;
- reads live `r6` as the channel id;
- records `RMCP01_GX_SET_TEV_ORDER`;
- rejects stages `>= GX_MAX_TEVSTAGE`;
- calls Aurora `GXSetTevOrder` with the four pinned enum conversions in the
  rendered fast-track;
- leaves synthetic/headless builds side-effect-free;
- exposes only PAL `0x8017214C`;
- adds `GXSetTevOrder hits` durable telemetry.

The synthetic probe uses zero values for `r3-r6` solely as
Nintendo-data-free compile/link coverage. Only hardware `r3 = 0` is claimed
from this run.

No neighboring TEV input/op/color/swap, texture, draw, DVD, or resource
boundary is pre-ported.

## Graphics state

The rendered graphics log remains at eleven FIFO writes. The final two events
remain:

```text
FIFO EVENT #10 size=1 value=0x00000061
FIFO EVENT #11 size=4 value=0x0f000000
```

This is still GX-state traffic, not drawable work. The periodic snapshot
records no display-list call, no FIFO-produced drawable work, no
`GXCopyDisp`, and no present.

## Preserved invariants

FST remains published at `0x97DC0000`, size 64,224 bytes / 2,096 entries.
The renderer remains initialized/frame-active and the translated watchdog stays
ACTIVE before the final blocker.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetTevOrder hits` must become non-zero;
2. `0x8017214C` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware, not from adjacent TEV symbols.
