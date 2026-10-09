# Checked nw4r layout quad bridge — 2026-10-10

The [fresh S10 trial](HARDWARE_TEV_S10_DRAW_QUAD_2026-10-10.md) reaches missing
native target `0x80084D20`. The pinned runtime names it
`nw4r::lyt::detail::DrawQuad`; its implementation constructs four vertices
with optional RGBA colors and zero to eight texture-coordinate arrays.
The source-owned missing-call registry now supplies this target on Switch.

The bridge validates the complete guest spans and current direct VAT0 layout
before changing native state. It accepts finite XY/F32 positions, finite size
and resulting extents, ST/F32 coordinates and optional RGBA8 colors. Negative
and zero sizes are supported. Unaligned and mapped-zero positions/size/UV
pointers are valid when the whole span is readable; color pointer zero means
no colors, as in the pinned wrapper. Aliases are read-only. Invalid counts,
incompatible layouts, partial/unmapped ranges, nonfinite values, an active
primitive/FIFO payload or guest/native display-list recording produce a
durable diagnostic abort before publication or FIFO output. Headless execution
also stops explicitly; a null CPU pointer is inert.

A bounded 307-byte stack packet preserves the pinned corner order 0,1,3,2,
single-precision `x + width` and `y - height`, texture-array stride 32 and
big-endian float/color words. RGB remains unchanged; alpha uses the low eight
bits of the modulation word and integer division by 255. All 65,536 alpha
pairs match the pinned multiply/magic-division helper. The bridge republishes
the HLE vertex descriptors and VAT0 to native GX, including the source layout,
and supplies the existing default alpha compare when needed. Native
`GXCallDisplayList` drains and decodes the borrowed packet synchronously, then
the frame is marked as containing work. CPU context, guest memory and the HLE
vertex state remain unchanged. It uses the pinned packet fallback available
in the Switch link; the desktop-only optimized submission API is not required.

`bash scripts/test-lyt-draw-quad.sh` executes both renderer modes under
ASan/UBSan. Its 386 valid packet cases compare production packets with the extracted pinned packet
builder and an independent byte oracle, exercises the actual native
`GXCallDisplayList` body, and verifies publication calls, dirty-state/primitive
flushes, preservation canaries and pre-output refusals. Unrelated registry
providers have abort-only link seams in this scoped test; their separate
contracts retain their actual native writers and decoders.

The desktop GPU suite links this production bridge and real checked Memory
slice into the existing Aurora replay harness. New textured and vertex-colored
quad scenes execute the actual native setters, display-list decoder and GPU
renderer. Interior and background pixel oracles check quad coverage; separate
capture/replay processes produce identical PNGs. Packet tests prove alpha
modulation; the selected-XFB presentation is opaque. The full existing GPU
suite also passes. Bridge/test changes now select the full replay CI job.

The exact private rendered-build and complete GitHub rollup gates are required
before merge and deployment. Hardware return and later game pixels are still
pending the new candidate's console trial. Generated code, game inputs, raw
reports and rendered NROs remain private.
