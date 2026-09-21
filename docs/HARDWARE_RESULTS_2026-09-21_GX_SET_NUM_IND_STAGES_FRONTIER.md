# Hardware result — GXSetNumIndStages frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch run of the rendered RMCP01 fast-track built from merged #206
progresses beyond PAL `GXSetNumTexGens (0x8016E5A4)` and exposes a new exact
unsupported DIRECT dispatch.

The final durable blocker is:

```text
kind   : DIRECT
target : 0x80171b38
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_NUM_TEX_GENS
action : abort after durable blocker record
```

The stage is set inside the merged GXSetNumTexGens bridge, so reaching the
distinct later target proves durable progression beyond `0x8016E5A4`.

The earlier periodic last-dispatch snapshot predates the final transition. It
therefore still records `GXSetNumTexGens hits = 0`; that timing fact does not
override the later blocker record.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that revision PAL `0x80171B38` is `GXSetNumIndStages`:

```cpp
extern "C" void GX__SetNumIndStages_80171b38(uint32_t n) {
    GXSetNumIndStages((u8)n);
}
```

The native-call catalog consumes only `cpu->gpr[3]`. Hardware captured
`r3 = 0`.

## Narrow Switch implementation

The candidate bridge:

- reads only `r3`;
- records `RMCP01_GX_SET_NUM_IND_STAGES`;
- narrows the value to `u8`;
- calls Aurora `GXSetNumIndStages` in the rendered fast-track;
- leaves synthetic/headless builds side-effect-free;
- exposes only PAL `0x80171B38`;
- adds `GXSetNumIndStages hits` durable telemetry.

No neighboring `GXSetTevDirect`, indirect-texture order/matrix/scale, TEV,
texture, draw, DVD, or resource boundary is pre-ported.

## Preserved invariants

The observed pre-blocker snapshot preserves a structurally valid FST,
initialized/active renderer, nine FIFO writes, and no display list, drawable
FIFO work, `GXCopyDisp`, or present.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetNumIndStages hits` must become non-zero;
2. `0x80171B38` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware, not from adjacent GX symbols.
