# PAL GXSetViewport — 0x801733B4

Tracking: #117, #162

## Hardware blocker

The first hardware run after #193 proves `TaskThread::run` and
`GXSetProjection`, then stops at:

```text
kind   : DIRECT
target : 0x801733B4
r1     : 0x80399008
r3     : 0x80399048
stage  : RMCP01_GX_SET_PROJECTION
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetViewport_801733b4`.

## Pinned semantics

The native override accepts:

```text
(float left, float top, float width, float height, float nearZ, float farZ)
```

It stores the six values in the runtime viewport cache and forwards them
unchanged to Aurora `GXSetViewport`.

The translator's declared-native ABI classifies these six scalar float
arguments as PPC `f1..f6`.

## Switch implementation

The Switch bridge reads the scalar values from `CpuContext::fpr[1..6].d`,
narrows them to host `float`, and forwards them to Aurora in the rendered
fast-track. Headless/synthetic builds keep the exact CPU/HLE boundary without
pulling Aurora into public CI.

No neighboring viewport/scissor/transform function is added speculatively.
The next hardware run remains the source of truth for the following boundary.

A dedicated `GXSetViewport hits` counter is added to the durable snapshots.
