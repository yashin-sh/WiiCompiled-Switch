# First I4 36×32 returned; second object frontier — 2026-10-07

The [first I4 36×32 candidate](GX_MII_I4_36X32_LOAD_2026-10-07.md) returns
through object `0x80397CC0`, slot 0, and its intervening Mii draw helper.
The next guarded load is **object `0x80397D00`**, slot 0, containing exactly
the same eight words and sharing the same 640-byte physical payload.
Target `0x80170F2C`, reason `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch
**632558 / 120.994 seconds**, LR **`0x800C45D0`**, restored stack
**`0x80397AE8`**, fiber **`0x80347498`**. The user reports **black screen,
test still running** as an intermediate observation; later diagnostics record
a durable intentional abort. Final visible outcome, GPU completion and
recognizable pixels remain unconfirmed.

## Exact run and report verification

Validated code is `4056328`; final Markdown-only head is
`f13db617672d13da02197d343f7eb6b1f58a7837`. PR #323 merges as
`c46f2ad0172801020b17ef122ea5802ed8a3de1d` after all five final-head workflows /
six actual jobs pass. All 22 local suites, seventeen mutants, rendered SDK
compilation, full synthetic/private builds, 71 required strong functions and
48 scoped unique providers pass. Original pins and upstream patch bytes/ns
mtimes stay preserved.

The NRO contains **73,621,560 bytes**, SHA-256
`e2ae9043f34efea6b3b0d59b72a306e8426e24dac6e4a6f5355fdedc284da136`.
Its SD copy is fully readback-verified. Netloader starts at **13:31:38 CEST**
and exits **0 at 13:32:04 CEST** (**11:31:38–11:32:04 UTC**), sending
**26,797,294 compressed bytes / 2,253 blocks (36.40%)**. Exact built inputs,
NRO hash, pinned dependencies and the upstream patch are checked before transfer.

USB/MTP retrieval at **13:35:24 CEST (11:35:24 UTC)** yields **37 reports /
632,902 bytes**, **16 changed / 21 retained**, versus the [RGB5A3 baseline](HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_LOAD_FRONTIER.md).
Every size, SHA-256 hash, baseline difference and raw ZIP member byte/CRC is
independently verified again. Source timestamps, runtime build ID, guest
bookkeeping words, native BP bytes and pixel content are unavailable. Private
NROs, generated products, game data and raw archives remain local-only.

## Return evidence and identical descriptor

The checked caller loads its first 36×32 object at LR `0x800C45A8`, then
calls draw helper `0x800C4B70` at LR `0x800C45C4`, then loads its second
object at LR `0x800C45D0`. The distinct second stop and restored caller stack
establish return through the first load and intervening helper. The fresh
post-main snapshot at 632550 records a later matrix dispatch inside the common
draw helper. Repeated discovery targets are deduplicated; no separate first
36×32 draw entry or retained first-load status is claimed. The load status is
overwritten by the second refusal.

The preceding heartbeat at 632300 records **5,995 FIFO writes, 102 successful
presents / zero failures**, six TaskThread hits, valid FST and coherent
scheduler state. This run's changed copy report records `copy-pass` at the new
aligned destination **`0x9210A740`**, checked bytes 32768, clear 1, 128×128 /
format 5. It is not a retained baseline copy report. The changed display-list
report records `end-pass`, base `0x921032E0`, capacity 64, bytes 32 and
save-context 1. These reports do not establish uploaded pixel content or GPU
completion. Neither elapsed nor dispatch-count differences prove performance.

Both 36×32 objects have:

`00000190 00000000 00007C23 0084E0BC 00000000 00000000 00000000 00140102`

I4 format 0, clamp/clamp, no mipmap, linear/linear zero-range LOD with edge
disabled, physical MEM2 data `0x109C1780`. Ceiling 8×8 / 32-byte tiling
requires **640 bytes**, including the partial right-edge tile; native logical
width stays 36. The [second-object correction](GX_MII_I4_36X32_SECOND_LOAD_2026-10-07.md)
admits only these two captured identities and keeps distinct native objects
for their shared payload. The second return remains unaccepted.
