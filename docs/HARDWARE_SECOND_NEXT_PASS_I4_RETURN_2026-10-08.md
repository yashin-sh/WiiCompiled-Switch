# Second next-pass Mii I4 return; RGB5A3 frontier — 2026-10-08

The corrected second next-pass Mii I4 load returns on the observed path.
The new durable stop is another `GXLoadTexObj` descriptor: RGB5A3 44×32 at
guest object `0x80397F40`, slot 0. This is native-load/helper progression,
not proof of recognizable game pixels or completion of the newly queued GPU work.

## Return attribution

The executed candidate is public code
`bc909cec6df5fe5514285993637698e998051bab`. Its private Rendered/Discovery
build has first-frame capture enabled. The exact locally validated product
was copied to SD with complete byte/hash readback and launched successfully
through Netloader. Product metadata and launch records remain local.

The previous blocker was I4 object `0x80397FC0`, source `0x109C1A40`, at
LR `0x800C4474`. The checked local caller loads `base + 416`, calls draw
helper `0x800C4B70` with return LR `0x800C449C`, then unconditionally loads
`base + 288` at LR `0x800C44A8`. The new stop has that latter LR and object
`0x80397F40`. Both objects agree with descriptor base `0x80397E20`.

This explicit caller inference, the distinct later descriptor and fresh
post-main matrix-load progress establish the second I4 native-load/helper
return. They do not provide individually captured success records for every
earlier occurrence or every relocated-source tuple. The overall GXLoadTexObj
API remains guarded for unsupported objects.

## Fresh reports and current stop

All 38 retrieved report files pass their recorded size/hash checks against the
local manifest. Thirteen differ from the preceding capture run and twenty-five
are byte-identical. MTP source timestamps are unavailable; attribution uses
the successful exact-product launch, changed blocker and coherent caller/runtime
evidence. Retained reports are not treated as newly observed operations.

Fresh heartbeat and post-main snapshots retain translated main, a structurally
valid FST, coherent guest/OS current/running identities and FIFO-produced work.
They record 102 successful presents and zero present failures. These counters
do not establish a new present after the accepted texture load.

The RGB5A3 descriptor matches the previously admitted 44×32 source tuple,
physical data `0x109C0C40`, word 3 `0x0084E062`, clamp/clamp, no mipmaps.
Its new guest identity is unadmitted, so the existing guard stops before native
loading. Its complete tiled range is 11×8×32 = 2,816 bytes. No correction or
native return for this new object is claimed here.

The retrieved first-frame capture and its successful desktop replay PNG are
byte-identical to the previous black, untextured first frame. They add no
visual progress or evidence about the later Mii scene.

## Validation

The candidate passes all 22 local contract suites, five focused mutation checks,
the private offline rendered build, the 65-bridge SDK syntax gate and scoped
provider/final ELF retention checks. All six public workflows / seven jobs
pass on the exact code revision, including desktop replay. Their run links
are recorded in [PR #332](https://github.com/yashin-sh/WiiCompiled-Switch/pull/332).
The subsequent hardware-result documentation changes no runtime/build/test inputs.

Raw reports, captures, game-derived images and private NRO metadata remain local.
See the [bounded correction](GX_MII_I4_SECOND_NEXT_PASS_LOAD_2026-10-08.md) and
[replay limits](DESKTOP_GX_REPLAY.md).
