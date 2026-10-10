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

The private rendered build and complete exact-HEAD CI rollup are required
before merging and deployment. The next corrected console run, subsequent
game pixels and playability remain pending. Private game data, translated
callers, NROs and raw reports are excluded from publication.
