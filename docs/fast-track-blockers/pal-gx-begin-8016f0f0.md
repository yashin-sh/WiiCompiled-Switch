# PAL GXBegin — 0x8016F0F0

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x8016f0f0
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000080
r4     : 0x00000000
r5     : 0x00000004
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_CULL_MODE
```

The distinct later target plus the stage written by the merged GXSetCullMode
bridge prove durable progression beyond PAL `0x8016F3B8`.

This is the first observed real RMCP01 draw-primitive boundary.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` consumes:

```text
r3 -> primitive
r4 -> GXVtxFmt
r5 -> vertex count
```

Hardware captured `0x80 / 0 / 4`.

The immediate path republishes the tracked Aurora vertex descriptor/format
state, initializes `g_hleGxState` for incremental vertex decoding, and leaves
the actual vertex bytes to `HleFifoWrite`.

## Switch implementation

Mirror the pinned display-list branch and deterministic begin-state setup.
Do not synthesize vertex payload and do not call a standalone Aurora draw in
place of the pinned FIFO-driven path.

The next proof is hardware-only: `GXBegin hits > 0`, durable progression
beyond `0x8016F0F0`, and inspection of `FIFO produced work`.

Do not speculatively port the following draw/state/copy boundary.
