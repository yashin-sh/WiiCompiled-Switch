# First relocated Mii I4 returned; second-object frontier — 2026-10-07

The [first relocated-source candidate](GX_MII_I4_RELOCATED_LOAD_2026-10-07.md)
returns through its first Mii load and intervening draw helper. Guarded
**GXLoadTexObj `0x80170F2C`** next rejects **`0x80397DC0`**, slot 0,
**I4 32×64**, physical data **`0x109C1A20`**. Reason
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR` records an intentional abort at
**dispatch 633149 / 122.796 seconds**, LR **`0x800C4474`**, stack
**`0x80397AE8`**. Next-pass object `0x80397F80` return remains unconfirmed.
The visual outcome is unavailable; recognizable game pixels and GPU completion
remain unaccepted.

## Candidate and verified retrieval

Code `1a8873c594e3169edc957fce319fce18b032846f`, final candidate head
`873495e4508a0cebab576199b6833b21fa9a41cb`, pass 22 suites, 40 compiled
mutations, SDK/synthetic/private builds and lint. All five final-head
workflows / six actual jobs pass before
[PR #330](https://github.com/yashin-sh/WiiCompiled-Switch/pull/330)
merges as `e72047d74f7cf98856250b3f6f66c147e9404a9a` at **17:24:26 CEST**
(15:24:26 UTC); the fetched merged tree equals the validated candidate.
Subsequent Markdown-only head `68388412cd9cd02a11304a8690f60345b2e9039a`
preserves every non-Markdown built input. The ELF retains 71 required strong
functions and 48 scoped unique providers across 235 host inputs, 19 Rust
archives and seven named libraries. Original pins and private upstream patch
bytes/ns mtimes stay preserved.

The private NRO contains **73,621,560 bytes**, SHA-256
`a87188951902f5836086eb47e834a171b44b666e1bd2558b77effb83ba52a5f1`.
Complete SD readback is verified. The same NRO transfers via Netloader
**17:29:10–17:30:50 CEST** (15:29:10–15:30:50 UTC), exit 0,
**26,797,667 compressed bytes / 2,253 blocks**. Transfer establishes transport.

At **17:33:25 CEST** (15:33:25 UTC), all **37 reports / 632,567 bytes**
are retrieved and independently checked against the preceding first-load
relocation run: **seven changed / thirty retained**. Every size, SHA-256,
baseline comparison and archive byte/CRC agrees. Changed reports are discovery,
blocker, load status, heartbeat/history, sleep events and post-main dispatch.
MTP source timestamps are unavailable; attribution uses the successful exact
transfer, fresh helper entry, distinct later descriptor and coherent execution.
Raw reports, archives, NROs and game data remain private.

## Accepted return through the pinned caller

Pinned `0x800C4300` copies r5 to its descriptor base, then executes the
unconditional first load at `base + 352`, slot 0, LR `0x800C444C`.
Its next draw-helper call `0x800C4B70` has LR `0x800C4468`, followed
unconditionally by the second load at `base + 416`, slot 0, LR `0x800C4474`.
The fresh caller first-hit at dispatch **632974** captures base
**`0x80397C20`**. Thus `base + 352 = 0x80397D80` and
`base + 416 = 0x80397DC0` agree with both runs.

A fresh helper first-hit at **633088 / LR `0x800C4468`** records the first
load's successor, absent from the preceding run. The later second-object
blocker restores the caller stack. This checked unconditional path establishes
first relocated-source native load **and helper return**, explicitly as caller
inference rather than individually captured success records. The last load
status is overwritten by refusal. Earlier source-tuple acceptance remains
scoped to its own runs; no next-pass or new second-source return is inferred.

The second object's complete descriptor is identical to the captured first
relocated object, including word 3 `0x0084E0D1`. Shared data must not collapse
native guest-object identity.

| Word | Value |
| --- | --- |
| 0 | `0x00000190` |
| 1 | `0x00000000` |
| 2 | `0x0000FC1F` |
| 3 | `0x0084E0D1` |
| 4 | `0x00000000` |
| 5 | `0x00000000` |
| 6 | `0x00000000` |
| 7 | `0x00200102` |

I4 requires `ceil(32/8) × ceil(64/8) × 32 = 1024` readable bytes at
`0x109C1A20`. Both format fields are zero; clamp/clamp, no mipmap,
linear/linear zero LOD, edge LOD disabled. Only this newly observed
identity/slot/tuple is added by the [bounded correction](GX_MII_I4_SECOND_RELOCATED_LOAD_2026-10-07.md).
No wider source-address interval or unobserved identity is admitted.

Copy/display-list reports are retained: native-Aurora GPU-only copy destination
`0x9210A720`, 32,768 checked bytes, clear 1, source 0,0,128,128,
destination 128,128, format 5, no mipmap; end-pass base `0x921032C0`,
capacity 64, bytes 32, save_context 1. Neither is fresh GPU/pixel evidence.
The fresh post-main snapshot at dispatch **633123**, target `0x80173214`,
LR `0x800C4BF8`, stack `0x80397A28`, is inside the intervening helper.
It records 5,995 FIFO writes, 102 present successes / zero failures, six
TaskThread runs, valid FST and coherent fiber/current/running identity
`0x80347498`. These counters prove neither recognizable pixels nor performance.
Second relocated-source return, next-pass return, GPU completion, controller/
audio completion and sustained gameplay remain open in issues #117 and #5.
