# Viewport/depth returned; Mii I4 texture frontier — 2026-10-07

The [viewport/depth candidate](GX_VIEWPORT_STATE_2026-10-06.md) returns through
both observed native boundaries in the Mii draw helper, then stops at guarded
**GXLoadTexObj `0x80170F2C`**, descriptor `0x80397D80`, slot 0. The durable
reason is `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch **631887 / 119.369
seconds**, LR `0x800C444C`, stack `0x80397AE8`, stage GX_LOAD_TEX_OBJ.

The user's intermediate observation is **black screen, test still running**.
The subsequently retrieved diagnostic records a deliberate abort after saving
this blocker. A final visible error is not separately confirmed. Recognizable
images, copied pixels and GPU completion remain unproven.

## Candidate and retrieval

Code `cc0d633417a58e3c53847598d65a6ccc12e8f2e8` passes 22 local suites, six
rejected mutants, both SDK modes, synthetic/private rendered builds and five
code-head workflows / six jobs. All five final-head workflows / six jobs also
pass on `c209b1f`; PR #319 merges as `378267f`.

The exact NRO is **73,621,560 bytes**, SHA-256
`73eb2313764fd89c8f777106b154ad7f6f80437161a623b01f68f94e8e905a16`.
Direct Netloader transfer starts at **07:52:41 UTC** and exits **0 at 07:53:15
UTC**, sending **26,799,100 compressed bytes / 2,253 blocks (36.40%)**.
Launch revision `c209b1fb03d0d91df04697346ab56c6095966b96` changes Markdown
only relative to built code. NRO/source hashes, dependency pins and the original
upstream patch are checked before transfer.

USB/MTP retrieval at **07:58:01 UTC** obtains **37 reports / 630,954 bytes**.
All sizes/hashes, baseline differences, ZIP member bytes and CRC are verified
again against the [PixModeSync baseline](HARDWARE_RESULTS_2026-10-06_PIX_MODE_SYNC_VIEWPORT_FRONTIER.md).
**Sixteen reports change; twenty-one are identical** and may be retained.
Source timestamps, runtime build ID, raw viewport outputs, depth registers and
pixels are unavailable. The exact successful launch and fresh coherent caller /
new guarded stop bind this run. Raw archives and private game products stay local.

## Observed progression

| Boundary | Dispatch | Captured state |
| --- | --- | --- |
| GXCopyTex `0x8016FD74` | 631748 | destination `0x9210A720`, clear 1, fresh native copy-pass / 32,768 bytes |
| GXPixModeSync `0x8016EB70` | 631761 | same Mii texture caller and fiber `0x80347498` |
| Mii draw helper `0x800C4300` | 631774 | caller stack `0x80397B48`, copy destination `0x9210A720` |
| GXGetViewportv `0x801733E0` | 631782 | output `0x80397B10`, stack `0x80397AE8` |
| Setup helper `0x800C3700` | 631783 | stage GX_GET_VIEWPORT, same output and stack |
| GXSetZScaleOffset `0x80173400` | 631804 | helper stack `0x80397A98`, LR `0x800C4334` |
| Guarded GXLoadTexObj `0x80170F2C` | 631887 | caller restored to `0x80397AE8`, LR `0x800C444C`, object `0x80397D80`, slot 0 |

The checked translated caller invokes the getter before the setup helper;
the setup helper invokes depth state and returns before this texture load.
The distinct later guarded stop and restored coherent caller establish both
returns. Acceptance covers this executed path only. Guest float bytes, screen
flags, depth mirror values and native XF writes are covered by host contracts,
not separate captured hardware outputs.

The preceding heartbeat at 631491 retains **5,988 FIFO writes, 102 successful
presents / zero failures**, six TaskThread hits, valid FST and coherent
fiber/current/running `0x80347498`. These counters do not identify pixels.
Elapsed time and dispatch totals are not performance comparisons.

## Exact next descriptor

The eight captured words are
`00000190 00000000 0000FC1F 0084E0D2 00000000 00000000 00000000 00200102`.
They describe **I4, 32×64, clamp/clamp, no mipmap**, slot 0, physical MEM2 data
`0x109C1A40`. The existing canonicalizer already preserves this physical range.
Pinned I4 uses 8×8 tiles of 32 bytes: 4×8 tiles need **1,024 bytes**. The refusal
report correctly records size zero while that descriptor is still unproven.

The [bounded I4 load candidate](GX_MII_I4_LOAD_2026-10-07.md) audits this one
complete descriptor and range before reusing native texture creation, LOD,
user-data and binding. It is not yet hardware-accepted. Other descriptors and
argument families keep their existing diagnostic guards.
