# PAL GXSetScissor — 0x80173430

Tracking: #117, #162

## Hardware blocker

The first hardware run after #194 crosses `GXSetViewport` and stops at:

```text
kind   : DIRECT
target : 0x80173430
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_VIEWPORT
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetScissor_80173430`.

## Pinned semantics

The native override accepts four unsigned PPC integer arguments:

```text
(left, top, width, height) = (r3, r4, r5, r6)
```

It mirrors guest-visible GX bookkeeping before calling Aurora:

```text
sx = left + 0x156
sy = top  + 0x156
ex = sx + width  - 1
ey = sy + height - 1

GXData + 0x148 = ((sx << 12) & 0x7ff000) |
                 (sy & 0x7ff) |
                 (old_start & 0xff800800)

GXData + 0x14c = ((ex << 12) & 0x7ff000) |
                 (ey & 0x7ff) |
                 (old_end & 0xff800800)

GXData + 0x002 = 0
GXSetScissor(left, top, width, height)
```

The runtime GXData pointer lives at guest address `0x803886C8`, already used
by the Switch GXInit and GXDrawDone bridges.

## Switch implementation

The Switch bridge reproduces the guest BP-word updates and dirty-halfword clear
with the existing endian-aware Memory API, then calls Aurora
`GXSetScissor` only in the rendered fast-track.

Synthetic/headless builds keep the native boundary and bookkeeping seam without
introducing an Aurora dependency.

A dedicated `GXSetScissor hits` counter is added to durable diagnostics.

No neighboring scissor-box-offset / viewport / matrix call is implemented
speculatively. The next hardware run remains the source of truth.
