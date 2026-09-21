# Hardware result — GXSetNumTevStages frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch run of the rendered RMCP01 fast-track built from merged #207
progresses beyond PAL `GXSetNumIndStages (0x80171B38)` and exposes a new exact
unsupported DIRECT dispatch.

The final durable blocker is:

```text
kind   : DIRECT
target : 0x801722a8
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_NUM_IND_STAGES
action : abort after durable blocker record
```

The stage is set inside the merged GXSetNumIndStages bridge, so reaching the
distinct later target proves durable progression beyond `0x80171B38`.

The periodic last-dispatch snapshot predates the final transition. It still
records `GXSetNumIndStages hits = 0`; that timing fact does not override the
later blocker record.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that revision PAL `0x801722A8` is `GXSetNumTevStages`:

```cpp
extern "C" void GX__SetNumTevStages_801722a8(uint32_t n) {
    if (n > GX_MAX_TEVSTAGE) {
        return;
    }
    GXSetNumTevStages((u8)n);
}
```

The native-call catalog consumes only `cpu->gpr[3]`. Hardware captured
`r3 = 1`, which is within the accepted count range.

## Narrow Switch implementation

The candidate bridge:

- reads only `r3`;
- records `RMCP01_GX_SET_NUM_TEV_STAGES`;
- preserves the pinned `n > GX_MAX_TEVSTAGE` rejection;
- narrows a valid value to `u8`;
- calls Aurora `GXSetNumTevStages` in the rendered fast-track;
- leaves synthetic/headless builds side-effect-free;
- exposes only PAL `0x801722A8`;
- adds `GXSetNumTevStages hits` durable telemetry.

No neighboring `GXSetTevOrder`, `GXSetTevDirect`, TEV input/op/color,
texture, draw, DVD, or resource boundary is pre-ported.

## Graphics progression

The rendered graphics log now records eleven FIFO writes. Events 10 and 11 are
new relative to the previous accepted nine-write frontier:

```text
FIFO EVENT #10 size=1 value=0x00000061
FIFO EVENT #11 size=4 value=0x0f000000
```

This is additional GX-state traffic, not yet proof of drawable work. No
display-list call, `GXCopyDisp`, or present has been established.

## Preserved invariants

FST remains published at `0x97DC0000`, size 64,224 bytes / 2,096 entries.
The renderer initializes successfully and the scheduler continues active
post-main translated execution.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetNumTevStages hits` must become non-zero;
2. `0x801722A8` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware, not from adjacent TEV symbols.
