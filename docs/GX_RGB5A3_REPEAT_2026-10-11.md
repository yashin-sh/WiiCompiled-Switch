# RGB5A3 repeat texture admission — 2026-10-11

The first LAN-debug run reaches dispatch 655,654 and pauses at
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, target `0x80170F2C`. The captured object
is a non-paletted RGB5A3 32 × 32 texture with linear filtering, repeat on both
axes, disabled edge LOD, zero LOD/bias and no mipmaps, user data or TLUT.

Its words are `00000195 00000000 00507C1F 008B7411 00000000 00000005
00000000 00400202`. RGB5A3 uses 4 × 4 tiles of 32 bytes; all 64 tiles require
2,048 readable bytes. The validator already accepts this exact family with
clamp, but restricts repeat to IA4/IA8. The correction admits repeat for
RGB5A3 through the same address-independent descriptor rule. Object/source
addresses remain identities, rather than an expanding address whitelist.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` native GX stores
both sampler modes independently of format, and loads the mode registers with
the texture metadata. The production bridge already forwards the validated
S/T modes and full dimension-derived backing span. No renderer or native GX
implementation changes are needed.

All other descriptor fields, legal binding slots, aligned readable objects,
full backing, tiled counts and overflow guards remain checked. I4 repeat,
mixed/mirror samplers and wrapped RGB5A3 counts remain outside this family.
The guest descriptor, payload and CPU state are preserved; native forwarding
must succeed before guest GX bookkeeping is published.

Production bridge/Memory contracts pass ASan, fatal UBSan and LeakSanitizer:
790 valid loads in each mode, with 5,319 diagnosed refusals headless and 5,355
rendered. They cover the captured literal, all eight slots, variable dimensions
and addresses, sampler refresh, forbidden-field mutations, shortened mappings
and native failure ordering. These seams validate admission/forwarding, not GPU
execution.

Fresh native GPU scenes render RGB5A3 32 × 32 clamp and repeat textures with
independent outside-range S/T samples. Opaque RGB5 red and A3RGB4 blue at alpha
3/7 exercise both decode branches and blending. Same-address data refresh and
separate-process replay produce byte-identical PNGs and pass independent pixel
oracles. The existing GPU suite, including partial-tile RGB5A3, also passes.

The local rendered NRO build succeeds with captures and Discovery disabled,
as in the preceding LAN-debug candidate. SDK/provider audit, public CI and
corrected hardware execution remain pending. Private game products, binaries
and raw diagnostics are excluded from publication.
