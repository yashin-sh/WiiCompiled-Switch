# PAL GXSetNumTexGens — 0x8016E5A4

Tracking: #117, #162

## Hardware blocker

The first rendered hardware run after the merged `GXSetChanCtrl` bridge
progresses to:

```text
kind   : DIRECT
target : 0x8016e5a4
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_CHAN_CTRL
```

The stage is written inside the `GXSetChanCtrl` bridge, so the distinct later
target proves this run progressed beyond the previous `0x80170570` frontier.
The earlier periodic liveness snapshot predates this final dispatch and is not
used as the final blocker record.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps the exact PAL entry point to:

```cpp
extern "C" void GX__SetNumTexGens_8016e5a4(uint32_t n) {
    GXSetNumTexGens((u8)n);
}
```

The native-call catalog passes only `cpu->gpr[3]`. The hardware run records
that value as zero.

## Switch implementation

The Switch bridge mirrors only that contract:

```text
count = r3
GXSetNumTexGens((u8)count)
```

It also records `RMCP01_GX_SET_NUM_TEX_GENS` and a dedicated dispatch hit
counter so the next hardware run can prove both entry and durable progression.

No neighboring texture-coordinate generator, texture-object, TEV, begin/draw,
or resource boundary is implemented speculatively.
