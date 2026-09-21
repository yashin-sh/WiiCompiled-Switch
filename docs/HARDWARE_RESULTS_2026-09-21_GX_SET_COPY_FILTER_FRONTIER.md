# Hardware result — GXSetCopyFilter frontier (2026-09-21)

Tracking: #117, #162

## Result

The latest rendered real-Switch RMCP01 run hardware-crosses
`EGG::AsyncDisplay::endRender (0x8020FF9C)` and exposes the next exact
unsupported DIRECT dispatch:

```text
kind   : DIRECT
target : 0x8016fa40
r1     : 0x80399088
r2     : 0x8038efa0
r3     : 0x00000000
r4     : 0x802457fe
r5     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_EGG_ASYNC_DISPLAY_END_RENDER
action : abort after durable blocker record
```

The durable post-main snapshot also records `AsyncDisplay endRender: 1`,
`GXBegin hits: 1`, 23 RMCP01 FIFO writes, and
`FIFO produced work: YES`. `GXCopyDisp` and present remain zero.

The stage is written inside the merged endRender bridge, so reaching the
distinct later target proves durable return/progression beyond `0x8020FF9C`.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x8016FA40` is:

```cpp
extern "C" void GX__SetCopyFilter_8016fa40(
    uint32_t aa, uint32_t spa, uint32_t vf, uint32_t vfa) {
    uint8_t sp[12][2]={}, vfb[7]={};
    if(spa) std::memcpy(sp, GuestToHostPtr(spa, 24), 24);
    if(vfa) std::memcpy(vfb, GuestToHostPtr(vfa, 7), 7);
    GXSetCopyFilter((GXBool)aa, sp, (GXBool)vf, vfb);
}
```

The PPC ABI is:

```text
r3 -> antialias enable
r4 -> sample-pattern guest pointer (24 bytes)
r5 -> vertical-filter enable
r6 -> vertical-filter guest pointer (7 bytes)
```

Hardware captured `r3 = 0`, `r4 = 0x802457FE`, `r5 = 1`.
The existing blocker format did not capture `r6`, so its value is not
inferred. The candidate reads live `r6` and extends blocker diagnostics to
capture it on the next run.

## Narrow Switch implementation

The candidate mirrors only this exact `GXSetCopyFilter` boundary:

- read live `r3..r6`;
- copy the pinned 24-byte sample pattern when `r4 != 0`;
- copy the pinned 7-byte vertical filter when `r6 != 0`;
- forward the two local arrays and GXBool values to Aurora
  `GXSetCopyFilter`;
- record `RMCP01_GX_SET_COPY_FILTER`;
- add `GXSetCopyFilter hits` telemetry;
- extend unsupported-blocker capture through `r6`.

The Nintendo-data-free synthetic probe is mapping/link-only because hardware
did not capture `r6`; it does not fabricate an argument tuple.

No `GXSetDispCopyGamma`, `GXCopyDisp`, copy destination, callback, or
present boundary is pre-ported.

## Preserved invariants

- PAL main reached.
- FST remains published at `0x97DC0000`, 64,224 bytes / 2,096 entries.
- renderer initialized and frame active.
- independent watchdog remains ACTIVE.
- `GXBegin hits = 1`.
- `AsyncDisplay endRender = 1`.
- 23 RMCP01 FIFO writes.
- `FIFO produced work = YES`.
- display-list calls = 0.
- `GXCopyDisp = 0`.
- present successes/failures = 0/0.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXSetCopyFilter hits > 0`;
2. durable blocker moves off `0x8016FA40`;
3. preserve `FIFO produced work = YES`;
4. preserve scheduler/FST/renderer invariants;
5. capture live `r6` in any following blocker;
6. record whether hardware next reaches `GXSetDispCopyGamma`,
   `GXCopyDisp`, or another exact boundary;
7. record the first present success/failure if it occurs.

The following boundary must come from hardware.
