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
as in the preceding LAN-debug candidate. Candidate `4297ed2` passes all eight
published checks on its exact HEAD. The NRO is 73,711,672 bytes,
SHA-256 `886856a610538c02d0246f2d2a1b0f830093f46ece04f54e406a12b007ab4365`.

## Verified LAN deployment and console return

The client replaces the previously installed candidate at the retained fixed
path `sdmc:/switch/WiiCompiled-Switch-debug-10dc30d.nro`. The basename still names
the earlier debug build; the full hash above identifies the new RGB5A3 code.
The previous `79b709bd…` candidate is backed up privately and size/SHA-256 verified.
Upload verification, complete SD readback, guarded rollback finalization and the
final current-only project NRO inventory succeed. The client receipt is complete,
and independent deployment verification passes. Backups and raw receipts stay
private.

The restart command is acknowledged, but the user manually opens the NRO for
the corrected trial. This run does not validate automatic restart into the
replacement candidate.

The fresh texture status is `load-pass` for object `0x80398EB8`, physical source
`0x116E8220`, RGB5A3 format 5, repeat/repeat and the full 2,048-byte backing span.
The next durable stop is DIRECT `GXSetTevKColorSel`, target `0x80171F30`, at
dispatch 656,213, with stage 0 and selector 31. Its `elapsed_ms=41207` measures
41.207 seconds from the first translated dispatch. The successful load status
and later distinct frontier establish return through this observed RGB5A3 load.

Live logs also record later signed TEV color, KColor, swap/order and TEVColorIn
stages. Those stage entries complement the successful texture status; they are
not individual native-return proofs. Fresh game pixels, menu progression and
playability remain unproven.

The [next bounded KSEL correction](GX_TEV_KSEL_2026-10-11.md) separates the
observed color-selector frontier from its conditional alpha-selector pre-port.
Private game products, binaries and raw diagnostics are excluded from publication.
