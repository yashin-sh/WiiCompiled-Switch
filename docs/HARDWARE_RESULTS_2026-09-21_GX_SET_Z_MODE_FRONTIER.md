# Hardware result — GXSetZMode frontier (2026-09-21)

Tracking: #117, #162

## Result

The latest rendered real-Switch RMCP01 run progresses beyond
PAL `GXSetAlphaUpdate (0x801727F8)` and exposes the next exact unsupported
DIRECT dispatch:

```text
kind   : DIRECT
target : 0x80172824
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_ALPHA_UPDATE
action : abort after durable blocker record
```

The stage is set inside the merged GXSetAlphaUpdate bridge, so reaching the
distinct later target proves durable progression beyond `0x801727F8`.

The existing blocker format did not record `r4` or `r5`. They must not be
invented from this run.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x80172824` is:

```cpp
extern "C" void GX__SetZMode_80172824(uint32_t ce, uint32_t f, uint32_t ue) {
    GXSetZMode(
        static_cast<GXBool>(ce),
        static_cast<GXCompare>(f),
        static_cast<GXBool>(ue));
}
```

The pinned ABI consumes live `r3/r4/r5`. Hardware captured only `r3 = 0`
in the current durable blocker.

## Narrow Switch implementation

The candidate bridge reads the three live guest registers at execution time:

```text
compare-enable = r3
compare-func   = r4
update-enable  = r5
```

It records stage `RMCP01_GX_SET_Z_MODE` and forwards exactly
`GXBool/GXCompare/GXBool` to Aurora in the rendered fast-track. Headless and
synthetic builds remain side-effect-free.

The Nintendo-data-free probe validates the mapping/link without fabricating
uncaptured `r4/r5` values.

The durable unsupported-dispatch report is extended to record `r4/r5` for
future multi-argument hardware frontiers.

No neighboring `GXSetZCompLoc`, pixel-format, dither, destination-alpha,
TEV, texture, draw, DVD, or resource boundary is pre-ported.

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

1. `GXSetZMode hits` must become non-zero;
2. `0x80172824` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. use the expanded blocker record for `r4/r5` on any later multi-argument
   boundary;
7. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware.
