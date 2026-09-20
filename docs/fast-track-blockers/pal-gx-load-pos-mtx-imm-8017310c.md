# PAL GXLoadPosMtxImm — 0x8017310C

Tracking: #117, #162

## Hardware blocker

The first hardware run after #195 crosses `GXSetScissor` and stops at:

```text
kind   : DIRECT
target : 0x8017310C
r1     : 0x80399008
r3     : 0x80399018
stage  : RMCP01_GX_SET_SCISSOR
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__LoadPosMtxImm_8017310c`.

## Pinned semantics

The native override accepts:

```text
matrix address = r3
matrix id      = r4
```

It reads a 3x4 matrix (12 big-endian float32 values / 48 bytes) from guest
memory, converts the values to host float order, and calls:

```text
GXLoadPosMtxImm(matrix, id)
```

No neighboring normal/texture/current-matrix function is required by the
current hardware evidence.

## Switch implementation

The Switch bridge validates the 48-byte guest matrix range, reads twelve
endian-aware float32 values through the existing guest-memory API, and forwards
the reconstructed host matrix plus matrix id to Aurora GX in the rendered
fast-track.

An invalid guest matrix range records a durable
`GX_LOAD_POS_MTX_IMM_INVALID_MATRIX` diagnostic and aborts instead of
fabricating success.

Synthetic/headless builds retain the direct native boundary without pulling
Aurora into public CI.

A dedicated `GXLoadPosMtxImm hits` counter is added to durable diagnostics.
