# Captured next-pass Mii RGB5A3 44×32 object — 2026-10-08

The [second next-pass I4 run](HARDWARE_SECOND_NEXT_PASS_I4_RETURN_2026-10-08.md)
stops at object `0x80397F40`, slot 0, RGB5A3 44×32, physical data
`0x109C0C40`, word 3 `0x0084E062`. Its complete descriptor matches the
earlier first-pass object. The correction admits only this new identity with
the existing exact eight-word descriptor and full 2,816-byte tiled range.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` maps the
target to `GX__LoadTexObj_80170f2c`, with object/map arguments from r3/r4.
RGB5A3 uses 4×4 tiles of 32 bytes: 11×8 tiles cover the entire 44×32 image.
Native dimensions remain 44×32, with clamp/clamp, linear filtering, disabled
edge LOD and no mipmaps. The Init → LOD → UserData → Load sequence is unchanged;
guest GX bookkeeping is published only after successful native work.

Native objects remain independently cached by guest identity even when they
share a payload. CPU registers, guest descriptor bytes and texture data remain
unchanged. Unknown objects or descriptors, short mappings, null pointers and
native exceptions remain diagnosed stops. No address range or guessed next
object is admitted.

The synthetic fixture adds repeated loads, independent host-object checks,
all 256 single-bit descriptor mutations, wrong slots/neighboring identities,
native failure ordering and CPU/guest preservation. Missing/31-byte descriptors
and missing/2048/2784/2815-byte payload mappings must stop. Fixtures contain
only fabricated data.

Targeted ASan/fatal UBSan/LSan contracts pass: 44 loads per mode,
4,078 headless and 4,108 rendered diagnosed refusals. Existing depth-LOD
contracts also pass. All 22 local suites and five focused mutation checks pass.
All six GitHub workflows / seven jobs pass on code
`c4066f9baa9bc4156b899fac35e67105bb1695dc`, including desktop replay pixel oracles.
The private offline rendered/capture build, 65-bridge SDK syntax gate,
scoped provider audit and final ELF retention pass. All nine original private
upstream patches preserve bytes and nanosecond mtimes.

A fresh Switch run accepts the observed native/helper return through the
checked unconditional caller sequence and distinct later I4 36×32 stop.
See the [hardware result](HARDWARE_NEXT_PASS_RGB5A3_RETURN_2026-10-08.md).
Recognizable game pixels and later-frame replay remain unproven.
Raw diagnostics, captures, images, private products and their metadata stay local.
