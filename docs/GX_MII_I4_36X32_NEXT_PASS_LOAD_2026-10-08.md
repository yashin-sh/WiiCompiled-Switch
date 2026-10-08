# Observed next-pass Mii I4 36×32 load — 2026-10-08

The [next-pass RGB5A3 run](HARDWARE_NEXT_PASS_RGB5A3_RETURN_2026-10-08.md)
stops at GXLoadTexObj `0x80170F2C`, object `0x80397EC0`, slot 0, I4 36×32,
physical source `0x109C1780`, word 3 `0x0084E0BC`. Its eight-word descriptor
matches the earlier 36×32 objects. This correction admits only the new observed
identity with the existing exact descriptor guard and full 640-byte tiled range.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` takes object/map
arguments from r3/r4. I4 uses 8×8 tiles of 32 bytes; ceil(36/8)×4 tiles require
640 bytes, including right-edge padding. Native width remains 36. Clamp/clamp,
linear filtering, disabled edge LOD and no mipmaps are unchanged. Native
Init → LOD → UserData → Load completes before guest GX bookkeeping is published.

Native objects remain independently cached by guest identity even when their
payload is shared. CPU registers, descriptors and texture data remain unchanged.
Unknown objects, modified words, wrong slots, short mappings, null pointers and
native exceptions remain diagnosed stops. Neighboring or guessed identities
are not admitted.

The synthetic fixture covers repeated loads and independent native identities,
all 256 single-bit descriptor mutations, wrong slots/neighboring objects,
native failure ordering, and CPU/guest preservation. Missing/31-byte descriptors
and missing/576/639-byte payload mappings must stop. Fixtures use fabricated data.
Targeted ASan/fatal UBSan/LSan contracts pass: 46 loads per mode, 4,350 headless
and 4,382 rendered diagnosed refusals.

Full local/CI gates, the private rendered build and a fresh hardware run are
required before accepting the new native return. Recognizable game pixels and
later-frame replay remain unproven. Raw reports, captures, images, generated
translations, private products and their metadata remain local/excluded.
