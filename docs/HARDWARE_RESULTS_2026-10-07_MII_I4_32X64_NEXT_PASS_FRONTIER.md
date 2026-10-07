# I4 16×16 returned; next Mii pass I4 frontier — 2026-10-07

The [I4 16×16 candidate](GX_MII_I4_16X16_LOAD_2026-10-07.md) completes the
previous Mii caller pass. The next guarded **GXLoadTexObj `0x80170F2C`**
uses a distinct object **`0x80397F80`**, slot 0, **I4 32×64**. Reason
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR` records an intentional abort at dispatch
**632832 / 120.862 seconds**, LR **`0x800C444C`**, stack **`0x80397AE8`**.
No final visual observation is available; recognizable game pixels and GPU
completion remain unconfirmed.

## Candidate and reports

Code `e0dd8e8` and final Markdown head
`4fe8f9a18da91a244a72aabd3f5fabfb5182af55` pass all 22 local suites,
33 compiled mutation checks, SDK/synthetic/private builds and lint.
All five final-head workflows / six actual jobs pass before
[PR #328](https://github.com/yashin-sh/WiiCompiled-Switch/pull/328) merges as
`a9fbe9d8dd1643a5b9e7dc02e0f653bd357bf196` at **16:12:22 CEST**
(14:12:22 UTC). The fetched merged tree is identical to the candidate.
The ELF retains 71 required strong functions and 48 scoped unique providers
across 235 host inputs, 19 Rust archives and seven named libraries. Original
pins and upstream patch bytes/ns mtimes stay preserved.

The private NRO contains **73,621,560 bytes**, SHA-256
`75686e0f994cd408aaa1c8a496e398d4fcb5c33fdf8e70b05e0e9c850ca2b1b0`.
Its complete SD byte/SHA readback is verified. The same NRO transfers via
Netloader **16:17:05–16:17:24 CEST** (14:17:05–14:17:24 UTC), exit 0,
**26,797,541 compressed bytes / 2,253 blocks**. Transfer alone does not
establish execution or pixels.

At **16:20:01 CEST** (14:20:01 UTC), all **37 reports / 632,236 bytes**
are retrieved and independently checked against the preceding second-RGB5A3
run: **thirteen changed / twenty-four retained**. Every size, SHA-256,
baseline comparison and archive byte/CRC agrees. Changed reports are DVD
status, discovery, blocker, wrap-mode/load status, heartbeat/history,
message/receive/sleep events, PAD, post-main dispatch and thread events.
MTP source timestamps are unavailable; attribution uses the exact successful
transfer, distinct current descriptor and coherent execution evidence.
Raw reports, archives, NROs and game data remain private.

## Accepted return and caller inference

The locally inspected pinned caller `0x800C4300` has an unconditional texture
load/draw sequence. At LR `0x800C4720` it loads the previously captured I4
16×16 object (`base + 480`), then invokes draw helper `0x800C4B70` at
LR `0x800C473C`. Later it repeats the seven load/draw pairs, including the
same 16×16 object at LR `0x800C4AF0`, and restores the caller stack.
The outer caller `0x800C2680` advances the descriptor base by **512 bytes**
after completing its size passes. The new stop is its first load
(`base + 352`, LR `0x800C444C`) at the next base: `0x80397E20`, instead of
`0x80397C20`. `0x80397C20 + 480 = 0x80397E00` and
`0x80397E20 + 352 = 0x80397F80` agree with both captured runs.

This verified control flow, distinct later object, restored stack/fiber and
fresh post-main progress establish previous 16×16/native-helper return and
completion of the prior caller pass. This is an explicit caller inference,
not separate captured success records for every repeated load. The final load
status is overwritten by refusal. The fresh post-main snapshot at dispatch
632805 reaches target `0x8017054C`, LR `0x800C4334`, stack `0x80397A98`,
consistent with the next pass's vertex-attribute setup before its first load.

| Word | Value |
| --- | --- |
| 0 | `0x00000190` |
| 1 | `0x00000000` |
| 2 | `0x0000FC1F` |
| 3 | `0x0084E0D2` |
| 4 | `0x00000000` |
| 5 | `0x00000000` |
| 6 | `0x00000000` |
| 7 | `0x00200102` |

Physical MEM2 data is **`0x109C1A40`**, shared with the two earlier 32×64
objects. I4 requires `ceil(32/8) × ceil(64/8) × 32 = 1024` bytes. Both
format fields are zero; clamp/clamp, no mipmap, linear/linear zero LOD,
edge LOD disabled. Native dimensions remain 32×64. Only this observed new
identity/slot/complete tuple is eligible for the
[next correction](GX_MII_I4_32X64_NEXT_PASS_LOAD_2026-10-07.md).

The copy and display-list reports are **retained**: copy-pass destination
`0x9210A720`, checked 32,768 bytes, clear 1, source 0,0,128,128,
destination 128,128, format 5, no mipmap, native-Aurora GPU-only;
end-pass base `0x921032C0`, capacity 64, bytes 32, save_context 1.
Neither is fresh GPU/pixel evidence. The preceding heartbeat records 6,004
FIFO writes, 102 present successes / zero failures, six TaskThread runs,
valid FST and coherent scheduler fiber `0x80347498`. The fresh post-main
snapshot records 6,012 FIFO writes. Timing and counters do not prove images
or performance. Next-object return, GPU completion, recognizable pixels,
controller/audio completion and sustained gameplay stay open in #117 and #5.
