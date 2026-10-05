# Sphere recording returned; PADRead frontier — 2026-10-04

The user confirms **black output followed by an error**. Recognizable Mario
Kart Wii pixels remain unproven. This run accepts both bounded GXDrawSphere
variants returning and reaches PADRead, rather than establishing list replay
or visible geometry.

## Candidate and retrieval

The rendered Discovery candidate contains code
`e7dd680603ba4c9d00fc673c12307ef099f3e0c0`; launch HEAD `b0817dc` only adds
validation documentation. All five exact-code workflows / six jobs and the
private build passed. The NRO is 73,478,200 bytes, SHA-256
`e2b0c3f283a0cf2e6e0b2c6bc15a6d431d0010ece269e1abcdb5ee09ead3ea50`.
Nxlink transferred 26,730,215 compressed bytes with exit 0 from
11:02:58 to 11:03:12 UTC. No TCP preflight consumed the netloader listener.

At 11:06:06 UTC, USB/MTP supplied **31 reports / 541,214 bytes**. Every size,
SHA-256, baseline difference and ZIP member byte/CRC was independently checked.
Thirteen reports changed and eighteen match the preceding SU run. Source
modification times were unavailable; unchanged files cannot independently date
an invocation. Private NROs, raw reports and diagnostic archives stay excluded.
Dependency pins and the existing nine-file submodule patch/mtimes are preserved.

## Accepted scope and remaining boundary

The first-hit record reaches sphere `(4,8)` at dispatch 606894, caller LR
`0x8021BEC8`. The new durable report then records `sphere-pass`, `(8,16)`,
6,591 bytes added and shared cursor 6,591. Checked constructor flow and the
later PADRead boundary establish both variants returned. The size matches the
untextured native layout; the report does not directly capture TEX0 descriptors.

The final End report is a **later** recording: buffer `0x80394F00`, capacity
16 KiB, `end-pass`, 32 bytes and save-context 1. Those 32 bytes are not the
sphere list size. Copied buffers, broader per-list lengths and replay are not
captured by these reports.

The new DIRECT blocker is **PADRead (`0x801AF44C`)**, dispatch **617831** /
**110.266 seconds**, LR `0x80543BE4`, stack `0x80399078`, status buffer
`0x9025F0D0`. The checked input caller passes its four-channel status array,
then calls the already translated PADClampCircle2 (`0x801AE7DC`). The primary
public RMCP01 symbol map at revision `94585b8a8fd7a2a52f30640ccff316e57880b6c1`
identifies PADRead explicitly; pinned WiiCompiled overrides it using Aurora's
desktop controller backend. A Switch input bridge is the next correction.
`RMCP01_GX_END_DISPLAY_LIST` remains the last annotated SDK stage, not evidence
that execution is still inside End.

## Progress without visual acceptance

The later snapshot records **3,937 guest FIFO writes**, up from 1,576, with
447 byte writes, 360 word writes and 3,130 float writes. There are 168 GXBegin
hits, 103 GXFlush hits, **99 preceding successful presents / zero failures**
and **zero display-list replay calls**. Native sphere writes are not counted
by the guest FIFO counter. Current/running guest thread `0x80347498` remains
coherent; six TaskThread runs and valid 64,224-byte FST are retained.

Nineteen DVD reads pass, including later face/avatar resources, save-banner
texture, UI font archive and WAD list. Loaded resources and present counters do
not prove visible font or game content. Input, subsequent SDK boundaries,
actual replay and recognizable images still need console validation; there is
no defensible fixed number of corrections or image ETA.

The [preceding result](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_SPHERE_FRONTIER.md)
and [sphere implementation](GX_DRAW_SPHERE_2026-10-04.md) retain their separate
validation scope.
