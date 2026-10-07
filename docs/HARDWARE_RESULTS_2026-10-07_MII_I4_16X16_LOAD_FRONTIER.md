# Second RGB5A3 38×32 returned; I4 16×16 frontier — 2026-10-07

The [second RGB5A3 38×32 candidate](GX_MII_RGB5A3_38X32_SECOND_LOAD_2026-10-07.md)
returns through object `0x80397C80`, its draw helper and intervening GX state
setup. The next guarded call is **GXLoadTexObj `0x80170F2C`**, object
**`0x80397E00`**, slot 0, **I4 16×16**. The durable reason is
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch **633125 / 122.353 seconds**,
LR **`0x800C4720`**, restored stack **`0x80397AE8`**, fiber **`0x80347498`**.
It records an intentional abort. No final visual observation is available;
recognizable game pixels and GPU completion remain unconfirmed.

## Candidate and report verification

Validated code is `63df50a`; final Markdown-only head is
`622854fcfd21eeae892bce5d7878dd9f044b8d4e`. PR #327 merges as
`0d4fff09140a6f627d1b0cd9cf4783ca164a943a` at **15:37:11 CEST**
(13:37:11 UTC) after all five final-head workflows / six actual jobs pass.
All 22 local suites, 27 mutation checks, rendered SDK compilation, full
synthetic/private builds, 71 strong functions and 48 scoped unique providers
pass. Original pins and upstream patch bytes/ns mtimes stay preserved.
The fetched merged main tree is identical to the validated candidate.

The private NRO contains **73,621,560 bytes**, SHA-256
`ccbf256986521c3d31e0634994eb56e87ff1bb34f858cb9307ddf9d40f96ae6a`.
Its SD copy has complete byte/SHA readback. The same candidate transfers via
Netloader **15:39:32–15:40:26 CEST** (13:39:32–13:40:26 UTC), exit 0,
**26,797,140 compressed bytes / 2,253 blocks**. Transfer success alone is
not execution or image evidence.

At **15:43:06 CEST** (13:43:06 UTC), all **37 reports / 632,976 bytes** are
retrieved. Every size, SHA-256, baseline comparison and archive byte/CRC is
independently verified: **thirteen changed / twenty-four retained** against
the preceding first-RGB5A3-38×32 run. Changed reports are DVD status, discovery,
blocker, wrap-mode/load status, heartbeat/history, message/receive/sleep events,
PAD status, post-main dispatch and thread events. MTP supplies no usable source
timestamps; attribution uses the exact transfer, coherent execution and distinct
new identity/descriptor. Reports, archives, NROs and game data stay private.

## Accepted return and new descriptor

The locally inspected pinned caller performs the second RGB5A3 load at
LR `0x800C4630`, draw helper `0x800C4B70` at LR `0x800C464C`, eight GX
state calls and then the new I4 load at LR `0x800C4720`. The sequence is
unconditional. The distinct later stop and restored caller stack establish
prior second-object, helper and intervening-call return. The final load report
is overwritten by refusal; no separate retained RGB5A3 success status is claimed.
The fresh post-main snapshot at dispatch 633109, target `0x80173214`,
LR `0x800C4BF8`, stack `0x80397A28`, agrees with the preceding draw helper.

| Word | Value |
| --- | --- |
| 0 | `0x00000190` |
| 1 | `0x00000000` |
| 2 | `0x00003C0F` |
| 3 | `0x0084E0F4` |
| 4 | `0x00000000` |
| 5 | `0x00000000` |
| 6 | `0x00000000` |
| 7 | `0x00040102` |

Physical MEM2 data is **`0x109C1E80`**. I4 uses 8×8 tiles of 32 bytes:
`ceil(16/8) × ceil(16/8) × 32 = 128`. Native width and height stay 16.
Both format fields are zero; clamp/clamp, no mipmap, linear/linear zero LOD
with edge LOD disabled. Only this observed identity/slot/complete tuple is
eligible for the [next correction](GX_MII_I4_16X16_LOAD_2026-10-07.md).

The copy report is **retained**, `copy-pass`, destination `0x9210A720`,
checked 32,768 bytes, clear 1, source 0,0,128,128, destination 128,128,
format 5, no mipmap, native-Aurora GPU-only. The display-list report is also
**retained**: end-pass base `0x921032C0`, capacity 64, bytes 32, save_context 1.
Neither is attributed as fresh pixels or GPU completion from this run.

The preceding heartbeat at dispatch 632788 records 5,995 FIFO writes,
102 present successes / zero failures, six TaskThread runs, valid FST and
coherent scheduler context. The later post-main snapshot reaches 6,001 FIFO
writes. These counters neither identify pixels nor measure performance.
I4 16×16 return, recognizable images, GPU completion, controller/audio
completion and sustained gameplay remain open in #117 and #5.
