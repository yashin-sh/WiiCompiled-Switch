# Second I4 36×32 returned; RGB5A3 38×32 frontier — 2026-10-07

The [second I4 36×32 candidate](GX_MII_I4_36X32_SECOND_LOAD_2026-10-07.md)
returns through object `0x80397D00`, slot 0, and its intervening Mii draw
helper. The new guarded call is **GXLoadTexObj `0x80170F2C`**, object
**`0x80397C40`**, slot 0: RGB5A3 **38×32** with a partial right-edge tile.
The durable reason is `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch
**631540 / 118.031 seconds**, LR **`0x800C4608`**, restored stack
**`0x80397AE8`**, fiber **`0x80347498`**. It records an intentional abort.
No final visual observation is available for this run. GPU completion and
recognizable game pixels remain unconfirmed.

## Candidate and report verification

Code is `5e65cdd`; final Markdown-only head is
`6308e58359623c5fdd186b9a576a7999e466c220`. PR #324 merges as
`1df611fe9405ccc955a1a35a525edd135ac587bc` at **14:06:35 CEST**
(12:06:35 UTC) after all five final-head workflows / six actual jobs pass.
All 22 local suites, nineteen mutation checks, rendered SDK compilation,
full synthetic/private builds and provider/retention audits pass. Original
pins and upstream patch bytes/ns mtimes are preserved.

The private NRO contains **73,621,560 bytes**, SHA-256
`bf093aeaccc848938d4477f9660bfc9256ad649b52e353016456533605145a75`.
The SD copy has complete byte/SHA readback. The same candidate transfers
through Netloader from **14:25:47 to 14:26:06 CEST** (12:25:47–12:26:06 UTC),
exit 0, **26,797,268 compressed bytes / 2,253 blocks**. Transfer success alone
is not execution or image evidence.

At **14:30:06 CEST** (12:30:06 UTC), all **37 reports / 632,811 bytes** are
retrieved. Every size, SHA-256, baseline comparison and archive byte/CRC is
rechecked. **Seven changed / thirty retained** against the preceding
first-36×32 run. Changed reports are discovery targets, blocker, texture load,
heartbeat/history, sleep events and post-main dispatch. MTP supplies no usable
source timestamps; attribution uses the exact transfer, coherent execution and
new object/descriptor frontier. Raw reports and archives stay excluded.

## Accepted return and observed descriptor

The locally inspected pinned caller performs the second 36×32 load at
LR `0x800C45D0`, then draw helper `0x800C4B70` at LR `0x800C45EC`, then an
intervening helper at LR `0x800C45FC`, and the new object load at LR
`0x800C4608`. These calls are unconditional in this sequence. The later stop
and restored caller stack establish prior return; the final texture-load
status file is overwritten by the new refusal, so there is no separate
retained second-I4 success status. The fresh post-main snapshot at dispatch
631510, target `0x80173214`, LR `0x800C4BF8`, stack `0x80397A28`, is inside
the intervening draw helper and agrees with this attribution.

| Word | Value |
| --- | --- |
| 0 | `0x00000190` |
| 1 | `0x00000000` |
| 2 | `0x00507C25` |
| 3 | `0x0084E010` |
| 4 | `0x00000000` |
| 5 | `0x00000005` |
| 6 | `0x00000000` |
| 7 | `0x00500202` |

Physical MEM2 data is **`0x109C0200`**. RGB5A3 uses 4×4 tiles of 32 bytes:
`ceil(38/4) × ceil(32/4) × 32 = 2560`. The unpadded 2,432 texel bytes do not
cover its partial edge tile. Native logical width stays 38. Both format fields
are 5; clamp/clamp, no mipmap, linear/linear zero LOD with edge LOD disabled.
Only this captured identity/slot/complete tuple is eligible for the
[next correction](GX_MII_RGB5A3_38X32_LOAD_2026-10-07.md).

The copy report is **retained**, `copy-pass`, destination `0x9210A740`,
checked 32,768 bytes, clear 1, source 0,0,128,128, destination 128,128, format 5,
no mipmap, native-Aurora GPU-only. The display-list report is also retained;
its end-pass has base `0x921032E0`, capacity 64, bytes 32, save_context 1.
Neither is fresh evidence of this run's pixels or GPU completion.

The preceding heartbeat at dispatch 631196 records 5,994 FIFO writes,
102 present successes / zero failures, six TaskThread runs, a valid FST and
coherent scheduler context. These counters neither identify pixels nor measure
performance. Recognizable images, broader controller/audio validation and
sustained gameplay remain open in issues #117 and #5.
