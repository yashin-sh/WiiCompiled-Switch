# Checked IA8 repeat texture family — 2026-10-10

The [fresh quad trial](HARDWARE_DRAW_QUAD_IA8_REPEAT_2026-10-10.md) accepts the
observed DrawQuad return and stops at a 32 × 32 IA8 descriptor with repeat on
both axes. Extend the existing address-independent, non-paletted texture
family to this sampler/format combination. Object and backing addresses stay
variable; all descriptor fields and the complete readable span are checked.

IA8 uses 4 × 4, 32-byte tiles. Exact words require linear filters, repeat S/T,
disabled edge LOD, zero LOD/bias, no mipmaps, no user data/TLUT and matching
format/dimensions/tile count. Counts exceeding the SDK's 15-bit field are
refused before native state changes. Existing I4/RGB5A3 clamp and legacy exact
descriptors keep their admission rules. Native GX initialization now forwards
the validated S/T modes instead of supplying clamp unconditionally.

ASan/UBSan contracts execute the production texture bridge and checked Memory
slice in both modes: 394 valid loads per mode, 4,162 diagnosed refusals
headless and 4,196 rendered. They cover the observed 2,048-byte span and all eight
binding slots, variable dimensions including partial tiles, relocated objects
and sources, same-object refresh, exact-span truncation and forbidden-field
mutations. Native-call seams verify format, dimensions, repeat S/T, linear
filters, LOD/edge settings and pointer length; they do not execute the GPU.

The fresh GPU scene uses the real pinned native IA8 decoder and repeat sampler
on a 32 × 32 tiled intensity pattern. UVs (1.25, -0.75) sample outside both
axes: clamp produces opposite pixels, so the white/black oracles verify repeat
behavior. The backing data is refreshed at the same address between draws.
Separate capture/replay processes produce byte-identical PNGs. The full
existing GPU suite, including the production DrawQuad scenes, also passes.
Texture bridge/descriptor/test changes now select full replay CI.

Candidate `e2aa724` passes the separate private rendered-build gate:
immutable offline rendered/capture build, 71 SDK HLE source files plus input,
backend, Discovery and capture checks, 85 unique providers across 273 inputs,
103 strong functions and three GX objects retained. Original private patches
preserve bytes and nanosecond mtimes. A private clamp mutant is rejected:
the GPU reports black where repeat requires white.

Every published check in the complete exact-HEAD rollup succeeds before
[PR #355](https://github.com/yashin-sh/WiiCompiled-Switch/pull/355) merges at
2026-10-10T09:03:33Z, as main `9a711ca`.
The private NRO is 74,436,664 bytes, SHA-256
`56b77d76afbe7c2c4348a809e515aacffd3107bd21ab125024a667990dab7086`. Complete SD readback verifies
`sdmc:/switch/WiiCompiled-Switch-ia8-repeat-e2aa724.nro` at
2026-10-10T09:03:48.461544+00:00; the capture-disabled marker is
independently reread. The prior owned quad candidate is backed up and verified
before removal. The next corrected console run, subsequent game pixels and
playability remain pending. Private game data, translated callers, NROs and
raw reports are excluded from publication.
