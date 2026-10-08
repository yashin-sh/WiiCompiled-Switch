# Next-pass Mii RGB5A3 return; I4 36×32 frontier — 2026-10-08

The corrected next-pass RGB5A3 44×32 load returns on the observed path.
The new durable stop is an unadmitted I4 36×32 descriptor at guest object
`0x80397EC0`, slot 0. This establishes native-load/helper progression;
recognizable game pixels and completion of newly queued GPU work remain unproven.

## Return attribution

The executed candidate is code `c4066f9baa9bc4156b899fac35e67105bb1695dc`,
with the private offline Rendered/Discovery build and first-frame capture enabled.
The exact validated product was copied to SD with complete byte/hash readback
and launched successfully through Netloader. Its public code and documentation
were merged in [PR #332](https://github.com/yashin-sh/WiiCompiled-Switch/pull/332).
Private product metadata and launch records remain local.

The previous stop was RGB5A3 object `0x80397F40`, source `0x109C0C40`,
at LR `0x800C44A8`. The checked local caller `0x800C4300`, attributed against
pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`, loads
`base + 288`, calls draw helper `0x800C4B70` with return LR `0x800C44C4`,
then performs the existing GX state calls before unconditionally loading
`base + 160` at LR `0x800C45A8`. The new stop has that latter LR and object
`0x80397EC0`. Both identities agree with descriptor base `0x80397E20`.
The locally checked caller hash matches the pre-build attribution record.

This explicit control-flow inference, the distinct later descriptor and fresh
post-main matrix-load progress establish RGB5A3 native-load, helper and
intervening-state-call return on this path. They do not provide individually
captured success records for every earlier occurrence or other source tuples.
The overall GXLoadTexObj API remains guarded for unsupported objects.

## Fresh reports and current stop

All 38 retrieved report files pass their recorded size/hash checks. Thirteen
changed against the preceding second-next-pass I4 run; twenty-five are
byte-identical. MTP timestamps are unavailable. Attribution uses the successful
exact-product launch, changed blocker and coherent caller/runtime evidence.
Retained reports are not treated as newly observed operations.

The fresh stop is `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, target `0x80170F2C`,
LR `0x800C45A8`, dispatch 637737 / 134.281 seconds after translated execution
began. Its I4 object is `0x80397EC0`, slot 0, 36×32, physical source
`0x109C1780`, word 3 `0x0084E0BC`, clamp/clamp and no mipmaps. The complete
tiled range is 5×4×32 = 640 bytes, including padding; logical width remains 36.
The descriptor guard stops before native loading. No correction or return for
this new identity is claimed here.

Fresh heartbeat and post-main snapshots retain translated main, a structurally
valid FST, coherent guest/OS current/running identities and FIFO-produced work.
They record 102 successful presents and zero failures. These counters do not
establish a new present after the accepted texture load.

The valid initial capture successfully replays on desktop. Its capture and
655×480 opaque-black PNG are byte-identical to the preceding run; they add no
visual progress or evidence about the later Mii scene.

## Validation

All 22 local contract suites, five focused mutation checks, the private offline
rendered build, 65-bridge SDK syntax gate and scoped provider/final ELF retention
checks pass. All six public workflows / seven jobs pass on the code revision
and final PR head before merge. Hardware-result documentation changes no
runtime, build or test inputs. The nine original private upstream patches
preserve bytes and nanosecond mtimes.

Raw reports, captures, game-derived images and private product metadata remain
local. See the [bounded correction](GX_MII_RGB5A3_NEXT_PASS_LOAD_2026-10-08.md)
and [replay limits](DESKTOP_GX_REPLAY.md).
