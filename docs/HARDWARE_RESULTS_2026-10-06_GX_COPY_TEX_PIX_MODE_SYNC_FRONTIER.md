# Native GXCopyTex returned; GXPixModeSync frontier — 2026-10-06

The [bounded GXCopyTex candidate](GX_COPY_TEX_2026-10-06.md) returns from the
observed RGB5A3 128×128 native copy. A fresh `copy-pass` report records cached
MEM2 destination `0x9210A720`, complete 32,768-byte preflight and clear 1.
Execution reaches the next DIRECT stop, **GXPixModeSync `0x8016EB70`**, at
**120.375 seconds**, stage `RMCP01_GX_COPY_TEX`. The user reports an **écran
noir followed by an error**; no error code was supplied. Recognizable game
images, GPU completion and copied pixels remain unproven.

## Candidate and retrieval

Validated code is `c18f2577c7b8ab09eec702b275a6f56df641afee`; launch revision
`0e96ca8b13f18a03ae69b83e941609ff27a5149a` differs only in Markdown.
The exact private NRO is **73,543,736 bytes**, SHA-256
`60b1b6649731195722fed9fd610ac7b34d489c93cc8387e9d55f36dccbbcb8c6`.
Candidate source hashes, dependency pins and original upstream patch are
checked before direct nxlink. Transfer starts at **18:27:14 UTC** and exits
**0 at 18:27:37 UTC**, sending **26,769,071 compressed bytes / 2,249 blocks
(36.40%)**. Earlier SD deployment has a complete matching readback.

USB/MTP retrieval starts at **18:30:43 UTC** and copies **37 reports /
629,452 bytes**. Every file size/hash, comparison against the
[configuration baseline](HARDWARE_RESULTS_2026-10-06_TEXTURE_COPY_CONFIG_COPY_TEX_FRONTIER.md),
ZIP member bytes and ZIP CRC is independently checked. **Thirteen reports
change; twenty-four are identical** and may be retained. MTP source timestamps
and runtime build ID are unavailable. Binding uses the verified exact launch,
new native-copy report and ordered later blocker. Private game products, NROs,
raw reports and archives remain excluded.

## Return evidence

The same Mii texture caller `RFLiSetupCopyTex` `0x800C2550` appears at dispatch
632151, fiber `0x80347498`, format 5, dimensions 128×128 and destination
`0x9210A720`. Clamp(3), Src(0,0,128,128) and Dst(128,128,5,0) precede the copy.
GXCopyTex and GXPixModeSync have consecutive first-hit records, both showing
dispatch **632170**, LR `0x800C3394` and stack `0x80397B18`. Explicit native
traits do not necessarily increment the shared translated dispatch counter;
these two entries are distinct targets, not evidence of a counter increase.

The checked caller invokes PixModeSync immediately after CopyTex. Its entry
stage has advanced to `RMCP01_GX_COPY_TEX`, the durable copy report is new,
and the current blocker is PixModeSync. Together these establish that the
observed native copy bridge returned. Acceptance is scoped to that source,
format, extent, cached destination and clear tuple. This run does not exercise
later destination reuse/retirement or prove general GPU cache correctness.

## Invariants and next boundary

The heartbeat at dispatch **631822** precedes this copy sequence. It records
six TaskThread hits, StaticR 4108, structurally valid FST, coherent guest
fiber/OS current/running `0x80347498`, active renderer/frame, **5,988 FIFO
writes**, **102 successful presents / zero failures**, and zero replay calls.
The DVD report contains successful reads through Mii/UI resources. These
snapshots do not capture copy-produced FIFO effects or identify present pixels.
Elapsed time is not a performance comparison with the prior run.

Pinned `runtime/src/hle/gx/gx_stubs.cpp` implements PixModeSync by attempting
the guest GXData halfword update at offset 2, then calling native GXPixModeSync.
Aurora's `GXManage.cpp` emits the current pixel-engine control BP register
and updates its sent-state flag. The next candidate must preserve this ordering
and best-effort guest-memory semantics; returning a no-op would omit the real
FIFO synchronization command. Its console return remains open.
