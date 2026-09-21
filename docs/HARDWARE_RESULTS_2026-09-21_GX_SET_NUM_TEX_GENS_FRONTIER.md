# Hardware result — GXSetNumTexGens frontier (2026-09-21)

Tracking: #117, #162

## Result

A real-Switch run of the rendered RMCP01 fast-track built from the post-#204
main line progresses beyond PAL `GXSetChanCtrl (0x80170570)` and exposes a new
exact unsupported DIRECT dispatch.

The rendered graphics path initialized successfully through Aurora GX, Dawn,
NVK and the Switch surface. The game reached `main`, the FST remained
structurally valid at `0x97DC0000` (64,224 bytes / 2,096 entries), and the
previous GX state-setup chain remained active. The periodic durable snapshot
still showed nine RMCP01 FIFO writes, no display-list calls, no drawable FIFO
work, no `GXCopyDisp`, and no present.

That periodic snapshot was written before the final dispatch. It therefore
still records `GXSetChanCtrl hits = 0`. The later durable blocker is stronger
control-flow evidence for this run: it records stage
`RMCP01_GX_SET_CHAN_CTRL`, which is set inside the implemented
`GXSetChanCtrl` bridge, then records a distinct later DIRECT target.

## New first blocker

```text
kind   : DIRECT
target : 0x8016e5a4
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_CHAN_CTRL
action : abort after durable blocker record
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, PAL `0x8016E5A4` is
`GXSetNumTexGens`.

Pinned semantics are exactly:

```cpp
GXSetNumTexGens((u8)n);
```

with `n = r3`. The hardware blocker records `r3 = 0`; no guest-memory
operand or additional register value is required.

## Narrow Switch implementation

The candidate Switch bridge:

- reads only `r3`;
- records stage `RMCP01_GX_SET_NUM_TEX_GENS`;
- narrows `r3` to `u8`;
- forwards it directly to Aurora `GXSetNumTexGens` in the rendered fast-track;
- keeps synthetic/headless builds side-effect-free;
- exposes exact PAL address `0x8016E5A4` through `KnownNativeCpuCall`;
- adds a durable `GXSetNumTexGens hits` counter.

No adjacent `GXSetTexCoordGen2`, draw, texture, TEV, DVD, or resource boundary
is pre-ported.

## Next hardware acceptance

After the candidate bridge passes public CI and the private rendered build gate:

1. `GXSetNumTexGens hits` must become non-zero;
2. `0x8016E5A4` must no longer be the durable DIRECT blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve the prior GX chain and scheduler/FST invariants;
5. record any first display list, drawable FIFO work, `GXCopyDisp`, present,
   DVD/resource activity, or native exception.

The following boundary must be chosen from that next hardware run rather than
from nearby GX symbols.
