# Second I4 load returned; Mii RGB5A3 frontier — 2026-10-07

The [second I4 candidate](GX_MII_I4_SECOND_LOAD_2026-10-07.md) returns through
object `0x80397DC0`, slot 0, then through the intervening draw helper and Mii
setup. The next guarded GXLoadTexObj is **object `0x80397D40`**, slot 0,
**RGB5A3 44×32**. Target `0x80170F2C`, reason
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch **635326 / 128.806 seconds**,
LR **`0x800C44A8`**, stack **`0x80397AE8`**, fiber **`0x80347498`**.
The user's intermediate observation is **black screen, test still running**;
the later diagnostic records an intentional abort after saving the blocker.
The final visible outcome, GPU completion and recognizable pixels remain
unconfirmed.

## Exact run and report verification

Validated code is `3256e810930a2b6c21bff3a3879e0909cf19cef9`, final
Markdown-only head `fde44c40ec39d1967af8d136cb32754122f175ad`. PR #321 merges
as `4da51002a9606c66a4700cefc23fc6c0ba6eea58` after all five final-head
workflows / six actual jobs pass. All 22 local suites, eight mutants, rendered
SDK gate, full synthetic/private builds, 71 required strong functions and
48 scoped unique providers pass; original pins and upstream patch/ns mtimes
remain preserved.

The NRO contains **73,621,560 bytes**, SHA-256
`fe2a28d984164f24f0a45d8d70dd5ee399dec040c33a89ff04bed31c46ef79cd`.
Its SD copy is verified by complete readback. Direct Netloader starts
**09:42:09 UTC** and exits **0 at 09:42:22 UTC**: **26,798,091 compressed
bytes / 2,253 blocks (36.40%)**. Exact built source/NRO hashes, pinned
submodules and existing upstream patch are checked before transfer.

USB/MTP retrieval at **09:44:53 UTC** yields **37 reports / 633,035 bytes**,
**13 changed / 24 retained**, versus the [first-I4 baseline](HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_LOAD_FRONTIER.md).
All sizes, SHA-256 hashes, baseline differences and raw ZIP member bytes/CRC
are independently verified again. Source timestamps, runtime build ID, native
BP bytes, guest bookkeeping words and pixel content are unavailable. Private
NROs, generated game products and raw archives remain local-only.

## Return evidence and next descriptor

The checked caller performs the second I4 load at LR `0x800C4474`, then calls
draw helper `0x800C4B70` at LR `0x800C4490`, then setup helper `0x800C38E0`
at LR `0x800C449C`, before loading its third object at LR `0x800C44A8`.
The fresh discovery report records setup helper `0x800C38E0` at dispatch
635298 on the restored caller stack and same fiber, followed by the distinct
third-load stop at 635326. This establishes second-load and intervening-helper
return. Repeated discovery targets are deduplicated; no separate second draw
entry or retained second-load status is claimed. The load report is overwritten
by the third refusal.

Getter/depth remain crossed at dispatches 635105 / 635121. The preceding
heartbeat at 635248 records **5,996 FIFO writes, 102 successful presents /
zero failures**, six TaskThread hits, valid FST and coherent scheduler state.
The copy report is byte-identical to the baseline and may be retained; these
counters and elapsed values do not identify pixels or prove performance.

The complete third descriptor is:

`00000190 00000000 00507C2B 0084E062 00000000 00000005 00000000 00580202`

This encodes RGB5A3 format 5, 44×32, clamp/clamp, no mipmap, linear/linear
zero-range LOD with edge disabled, physical MEM2 payload `0x109C0C40`.
Pinned native tiling uses 4×4 texels / 32 bytes: 11×8 tiles require
**2,816 bytes**. Its descriptor had not been observed in the previous run;
it was deliberately guarded until this capture. The
[bounded correction](GX_MII_RGB5A3_LOAD_2026-10-07.md) admits only this complete
observed object/slot/descriptor and full range. Its return remains unaccepted.
