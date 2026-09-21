# Hardware result — GXSetCullMode frontier (2026-09-21)

Tracking: #117, #162

## Result

The latest rendered real-Switch RMCP01 run progresses beyond
PAL `GXSetZMode (0x80172824)` and exposes the next exact unsupported
DIRECT dispatch:

```text
kind   : DIRECT
target : 0x8016f3b8
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000002
r4     : 0x00000000
r5     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_Z_MODE
action : abort after durable blocker record
```

The stage is set inside the merged GXSetZMode bridge, so reaching the
distinct later target proves durable progression beyond `0x80172824`.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x8016F3B8` is:

```cpp
extern "C" void GX__SetCullMode_8016f3b8(uint32_t m) {
    GXSetCullMode(static_cast<GXCullMode>(m));
}
```

The pinned ABI consumes only live `r3`. Hardware captured `r3 = 2`.

## Narrow Switch implementation

The candidate bridge reads live `r3`, records
`RMCP01_GX_SET_CULL_MODE`, and in the rendered fast-track forwards exactly
`static_cast<GXCullMode>(r3)` to Aurora `GXSetCullMode`.
Synthetic/headless builds remain side-effect-free.

A Nintendo-data-free synthetic probe uses `r3 = 2`, matching the captured
hardware scalar.

No neighboring `GXSetCoPlanar`, `GXSetClipMode`, pixel-state, texture,
draw, DVD, or resource boundary is pre-ported.

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

1. `GXSetCullMode hits` must become non-zero;
2. `0x8016F3B8` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. retain or advance the eleven-write FIFO state;
6. record the first display list, drawable work, `GXCopyDisp`, present,
   resource/DVD event, or native exception if any appears.

The following boundary must come from hardware.
