# First RGB5A3 38×32 returned; second object frontier — 2026-10-07

The [RGB5A3 38×32 candidate](GX_MII_RGB5A3_38X32_LOAD_2026-10-07.md)
returns through object `0x80397C40`, slot 0, and its intervening Mii draw
helper. The next guarded load is **object `0x80397C80`**, slot 0, containing
exactly the same eight words and sharing the same 2,560-byte physical payload.
Target `0x80170F2C`, reason `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch
**633974 / 125.572 seconds**, LR **`0x800C4630`**, restored stack
**`0x80397AE8`**, fiber **`0x80347498`**. It records an intentional abort.
No final visual observation is available for this run; recognizable game pixels
and GPU completion remain unconfirmed.

## Exact run and verification

Validated code is `ad75948`; final Markdown-only head is
`89d2d401713721f3a8df34197bed90d839423e7b`. PR #326 merges as
`101fee454d3f4b19b86c7998e400db3530739b6e` at **14:59:04 CEST**
(12:59:04 UTC) after all five final-head workflows / six actual jobs pass.
All 22 local suites, twenty-five compiled mutations, rendered SDK compilation,
full synthetic/private builds, 71 strong functions and 48 scoped unique
providers pass. Original pins and upstream patch bytes/ns mtimes stay preserved.
The merged tree preserves every candidate file and the separately merged
Strikers reference Markdown document; executable inputs are unchanged.

The private NRO contains **73,621,560 bytes**, SHA-256
`4fd7e46ef4aa09bab2102bfa8cdc784dba061a0ef385b89922597d5dcd861d2d`.
Its SD copy has complete byte/SHA readback. The same candidate transfers via
Netloader from **15:07:11 to 15:07:34 CEST** (13:07:11–13:07:34 UTC), exit 0,
**26,798,256 compressed bytes / 2,253 blocks**. Transfer establishes neither
execution correctness nor game images on its own.

At **15:10:19 CEST** (13:10:19 UTC), all **37 reports / 632,323 bytes** are
retrieved. Every size, SHA-256, baseline comparison and archive byte/CRC is
independently verified. **Sixteen changed / twenty-one retained** against the
second-36×32 run. Changed reports include discovery, blocker, texture load,
copy/display-list, wrap-mode, heartbeat/history, DVD/SZS, message/sleep/thread
and PAD reports. MTP supplies no usable source timestamps; attribution uses
the exact transfer, coherent execution and distinct new identity. Reports,
archives, NROs and game data stay private.

## Accepted first return and new tuple

The locally inspected pinned caller performs the first RGB5A3 38×32 load at
LR `0x800C4608`, calls draw helper `0x800C4B70` at LR `0x800C4624`, then
loads the second object at LR `0x800C4630`. This sequence is unconditional.
The distinct later stop and restored caller stack establish first-object and
helper return. The final load report is overwritten by refusal; no separate
retained first-RGB5A3 success status is claimed. The fresh post-main snapshot
at dispatch 633930, target `0x80173214`, LR `0x800C4BF8`, stack `0x80397A28`,
is inside that intervening helper and agrees with this attribution.

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

RGB5A3, 38×32, physical MEM2 **`0x109C0200`**, clamp/clamp, no mipmap,
linear/linear zero LOD with edge LOD disabled. Both objects require all
**2,560 bytes** including the partial right-edge tile; native logical width
stays 38. Only these two observed identities may use the complete exact tuple.
The [next correction](GX_MII_RGB5A3_38X32_SECOND_LOAD_2026-10-07.md) retains
independent native host objects despite the shared payload.

The **changed** copy report is `copy-pass`, destination **`0x9210A720`**
(previously `0x9210A740`), checked 32,768 bytes, clear 1, source 0,0,128,128,
destination 128,128, format 5, no mipmap, native-Aurora GPU-only. The **changed**
display-list end-pass has base **`0x921032C0`** (previously `0x921032E0`),
capacity 64, bytes 32, save_context 1. These differ from baseline, but neither
establishes GPU completion or recognizable pixels.

The preceding heartbeat at dispatch 633433 records 5,988 FIFO writes,
102 present successes / zero failures, six TaskThread runs, valid FST and
coherent scheduler context. The later post-main snapshot reaches 6,000 FIFO
writes. These counters do not identify pixels or measure performance. New
second-object return, GPU completion, recognizable game images, controller/audio
completion and sustained gameplay remain open in #117 and #5.
