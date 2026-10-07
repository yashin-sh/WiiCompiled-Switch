# First Mii I4 load with relocated data — 2026-10-07

The exact [next-pass candidate](GX_MII_I4_32X64_NEXT_PASS_LOAD_2026-10-07.md)
from [PR #329](https://github.com/yashin-sh/WiiCompiled-Switch/pull/329)
reaches a different **first Mii load** tuple. Guarded **GXLoadTexObj
`0x80170F2C`** rejects object **`0x80397D80`**, slot 0, **I4 32×64**,
physical data **`0x109C1A20`**, reason `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`.
The intentional abort records dispatch **634566 / 126.826 seconds**,
LR **`0x800C444C`**, stack **`0x80397AE8`**. This run does **not** establish
return from the next-pass object `0x80397F80`; that candidate remains pending
hardware acceptance. Visual outcome is unavailable; recognizable game pixels
and GPU completion remain unconfirmed.

## Candidate and verified retrieval

Code `551cdbf74942fde66683f192af1bf89004e7980d`, final Markdown head
`0ed69bc04ebd03e00fea5c986ebe8804cb818f71`, pass the 22-suite gate,
35 compiled mutations, SDK/synthetic/private rendered builds and lint.
All five final-head workflows / six actual jobs pass before PR #329 merges
as `19a2dd4f38a212806a51dc59675c88909dedf06e` at **16:44:03 CEST**
(14:44:03 UTC). The fetched merged tree equals the validated candidate.
The ELF retains 71 required strong functions and 48 scoped unique providers.
Original dependency pins and private patch bytes/ns mtimes stay preserved.

The private NRO contains **73,621,560 bytes**, SHA-256
`0b30ee1789163164671f6cb37e7a150dc6e3851e8c7875bdc99fb8cbe7437b24`.
Its complete SD readback is verified. The exact NRO transfers via Netloader
**16:47:51–16:48:04 CEST** (14:47:51–14:48:04 UTC), exit 0,
**26,798,440 compressed bytes / 2,253 blocks**. Transfer proves transport.

At **16:52:15 CEST** (14:52:15 UTC), all **37 reports / 631,804 bytes**
are retrieved and independently checked against the preceding I4 16×16 run:
**fourteen changed / twenty-three retained**. Every size, SHA-256, baseline
comparison and archive byte/CRC agrees. Changed reports are DVD status,
discovery, blocker, Init/wrap/load status, heartbeat/history, message/receive/
sleep events, PAD, post-main dispatch and thread events. MTP source timestamps
are unavailable; attribution uses the exact successful transfer, distinct
current descriptor and coherent fresh execution. Raw reports, archives,
NROs and game data remain private.

## First-load attribution and acceptance limits

The pinned caller `0x800C4300` copies r5 to its descriptor base, then calls
GXLoadTexObj unconditionally with `base + 352`, slot 0, LR `0x800C444C`.
The fresh first-hit caller at dispatch **634459** captures r5 **`0x80397C20`**,
so `0x80397C20 + 352 = 0x80397D80` matches the blocker. The subsequent draw
helper `0x800C4B70`, called only after this load returns, has **no first-hit
entry in this run**. The previous run recorded it at dispatch 631932 and
later stopped at next-pass object `0x80397F80`. This run's higher dispatch
count does not establish later Mii drawing: its fresh snapshot has **5,995
FIFO writes / 242 GXBegin hits**, versus the previous **6,012 / 258**.
Historical accepted returns stay scoped to their earlier captured tuples.

Only word 3 changes in the first object's descriptor: `0x0084E0D2` becomes
`0x0084E0D1`, decoding a data address 32 bytes earlier. The latest initialization
status also records object `0x80398000`, I4 16×16, data `0x909C1E60` rather
than `0x909C1E80`, another 32-byte difference. The cause of the address
variation is unproven; no allocation policy or broad pointer range is inferred.

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

The complete I4 tiled range remains `ceil(32/8) × ceil(64/8) × 32 = 1024`
bytes. Both format fields are zero; clamp/clamp, no mipmap, linear/linear,
zero LOD, edge LOD disabled. Only the first captured identity/slot/tuple is
eligible for the [bounded correction](GX_MII_I4_RELOCATED_LOAD_2026-10-07.md).
Other objects and other shifted data addresses remain unproven.

Copy/display-list reports are retained: copy destination `0x9210A720`,
32,768 checked bytes, clear 1, source 0,0,128,128, destination 128,128,
format 5, no mipmap, native-Aurora GPU-only; end-pass base `0x921032C0`,
capacity 64, bytes 32, save_context 1. Neither is fresh GPU/pixel evidence.
Fresh liveness records six TaskThread runs, valid FST, coherent fiber/current/
running identity `0x80347498`, 102 present successes / zero failures.
These counters prove neither recognizable pixels nor performance.
Relocated first-load return, next-pass return, GPU completion, controller/audio
completion and sustained gameplay remain open in issues #117 and #5.
