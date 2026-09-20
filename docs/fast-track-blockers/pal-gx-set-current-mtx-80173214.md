# PAL GXSetCurrentMtx — 0x80173214

Tracking: #117, #162

## Hardware blocker

The first hardware run after #196 crosses `GXLoadPosMtxImm` and stops at:

```text
kind   : DIRECT
target : 0x80173214
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_LOAD_POS_MTX_IMM
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetCurrentMtx_80173214`.

## Pinned semantics

The native override accepts:

```text
matrix id = r3
```

and calls:

```text
GXSetCurrentMtx(id)
```

No guest-memory conversion or neighboring matrix-loader implementation is
required by the current hardware evidence.

## Switch implementation

The Switch bridge reads the PPC matrix id from `r3`, records stage
`RMCP01_GX_SET_CURRENT_MTX`, and forwards the id directly to Aurora GX in the
rendered fast-track.

Synthetic/headless builds retain the native direct-call boundary without
pulling Aurora into Nintendo-data-free public CI.

A dedicated `GXSetCurrentMtx hits` counter is added to durable diagnostics.
