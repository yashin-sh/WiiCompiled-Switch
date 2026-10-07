# RGB5A3 load returned; I4 36×32 frontier — 2026-10-07

The [RGB5A3 candidate](GX_MII_RGB5A3_LOAD_2026-10-07.md) returns through
object `0x80397D40`, slot 0, and its intervening Mii draw helper. The next
GXLoadTexObj stop is **object `0x80397CC0`**, slot 0, **I4 36×32**. Target
`0x80170F2C`, reason `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch
**634832 / 127.328 seconds**, LR **`0x800C45A8`**, stack **`0x80397AE8`**,
fiber **`0x80347498`**. The user reports **black screen, test still running**
as an intermediate observation; the later diagnostic records a durable
intentional abort. Final visible outcome, GPU completion and recognizable
pixels remain unconfirmed.

## Exact run and report verification

Validated code is `e6a4727`; final Markdown-only head is
`9f1f6ba64b08d9377ad64cd4678cace0d8a5cc47`. PR #322 merges as
`893812232d5f804907b4c3317cafbf4160942f68` after all five final-head workflows /
six actual jobs pass. All 22 local suites, twelve mutants, rendered SDK gate,
full synthetic/private builds, 71 required strong functions and 48 scoped
unique providers pass. Original pins and upstream patch bytes/ns mtimes stay
preserved.

The NRO contains **73,621,560 bytes**, SHA-256
`4794cdc6ad5b3a203b6eb86a1d77c3a6c1433d7e5169f85c700b6e30ca5b9641`.
Its SD copy is fully readback-verified. Direct Netloader starts
**10:16:53 UTC** and exits **0 at 10:17:06 UTC**, sending **26,798,236
compressed bytes / 2,253 blocks (36.40%)**. Exact built inputs, NRO hash,
pinned dependencies and the upstream patch are checked before transfer.

USB/MTP retrieval at **10:20:55 UTC** yields **37 reports / 632,748 bytes**,
**13 changed / 24 retained**, versus the [second-I4 baseline](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_LOAD_FRONTIER.md).
Every size, SHA-256 hash, baseline difference and raw ZIP member byte/CRC is
independently verified again. Source timestamps, runtime build ID, guest
bookkeeping words, native BP bytes and pixel content are unavailable. Private
NROs, generated products, game data and raw archives stay local-only.

## Return evidence and padded texture range

The checked caller unconditionally loads RGB5A3 at LR `0x800C44A8`, calls draw
helper `0x800C4B70` at LR `0x800C44C4`, configures the following TEV state and
then loads its next object at LR `0x800C45A8`. The fresh fourth-load stop,
restored caller stack and same fiber establish return through RGB5A3 and its
intervening helper. Discovery targets are deduplicated; no separate third-draw
entry or retained RGB5A3 status is claimed. The load status is overwritten by
the fourth refusal.

Getter/depth remain crossed at dispatches 634511 / 634533. The preceding
heartbeat at 634666 records **5,996 FIFO writes, 102 successful presents /
zero failures**, six TaskThread hits, valid FST and coherent scheduler state.
The copy report is byte-identical to the baseline and may be retained. Neither
these counters nor differing elapsed/dispatch counts establish pixels or
performance.

The complete fourth descriptor is:

`00000190 00000000 00007C23 0084E0BC 00000000 00000000 00000000 00140102`

This encodes I4 format 0, 36×32, clamp/clamp, no mipmap, linear/linear
zero-range LOD with edge disabled, physical MEM2 payload `0x109C1780`.
Pinned native tiling is 8×8 texels / 32 bytes: ceil(36/8) × ceil(32/8) =
5×4 tiles, requiring **640 bytes**. The unpadded 576-byte texel count omits
part of the right-edge tile. Keep the logical width at 36 while checking the
full tiled range. The [bounded correction](GX_MII_I4_36X32_LOAD_2026-10-07.md)
admits only this exact captured object/slot/descriptor. Its return remains
unaccepted.
