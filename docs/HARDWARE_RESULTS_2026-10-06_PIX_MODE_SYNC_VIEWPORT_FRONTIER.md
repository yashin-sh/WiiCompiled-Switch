# PixModeSync returned; viewport getter frontier — 2026-10-06

The [PixModeSync bridge](GX_PIX_MODE_SYNC_2026-10-06.md) returns on the Mii
texture path. Execution resumes inside the caller and reaches **GXGetViewportv
`0x801733E0`**, DIRECT, dispatch **635096 / 128.396 seconds**, stage
`RMCP01_GX_INVALIDATE_TEX_ALL`. This run's visual observation remains pending;
recognizable game images and GPU completion remain unproven.

## Candidate and evidence

Validated code is `bed9d2965a0c3add48d702c1bdeed713166f2802`; launch revision
`b38a69aff9130a85b213556e8a65f2b26a06a2bf` differs only in Markdown. All
21 suites, five exact-code workflows / six jobs, both SDK modes and the
private rendered build pass; 65 required strong functions and 39 scoped
unique providers are checked. PR #318 additionally passes all five workflows /
six jobs at final HEAD `f37f401` and is merged as `7b41350`.

The exact NRO is **73,556,024 bytes**, SHA-256
`fefaaf0e40a9b6553f18746ae64d8b1dfdb6708f1053c7537b981a16f0c630df`.
Direct nxlink starts at **19:14:05 UTC**, exits **0 at 19:14:18 UTC**, and
sends **26,774,763 compressed bytes / 2,249 blocks (36.40%)**. Candidate
hashes, pins and upstream patch are checked before transfer. No SD deployment
is claimed for this candidate.

USB/MTP retrieval at **19:27:46 UTC** verifies **37 reports / 631,739 bytes**.
Every size/hash, baseline comparison, ZIP member byte and ZIP CRC is independently
checked against the [preceding copy run](HARDWARE_RESULTS_2026-10-06_GX_COPY_TEX_PIX_MODE_SYNC_FRONTIER.md).
**Fifteen reports change; twenty-two are identical** and may be retained. Source
timestamps, runtime build ID, raw guest mirrors and output pixels are unavailable.
The exact successful launch and coherent fresh caller/next-blocker progression
bind the run. Private NROs, game products and raw archives stay excluded.

## Observed progression

| Target | Dispatch | Captured state |
| --- | --- | --- |
| RFLiSetupCopyTex `0x800C2550` | 635044 | RGB5A3 128×128, destination `0x9210A740`, fiber `0x80347498` |
| GXCopyTex `0x8016FD74` | 635063 | destination `0x9210A740`, clear 1; fresh native copy-pass, 32,768 bytes |
| GXPixModeSync `0x8016EB70` | 635076 | LR `0x800C3394`, stack `0x80397B18`, stage GX_COPY_TEX |
| Mii continuation `0x800C3DE0` | 635088 | restored caller stack `0x80397B48`, stage GX_SET_SCISSOR |
| Mii draw helper `0x800C4300` | 635095 | LR `0x800C3508`, same fiber, stage GX_SET_SCISSOR |
| GXGetViewportv `0x801733E0` | 635096 | output `0x80397B10`, stack `0x80397AE8`, stage GX_INVALIDATE_TEX_ALL |

The checked caller invokes PixModeSync immediately after native copy, then
unwinds to the caller that reaches the distinct Mii continuation and draw
helper. This later coherent path and a different current blocker establish
PixModeSync returned. The fresh copy report additionally accepts the same
bounded shape/clear at a second cached destination. Raw guest-halfword writes,
individual BP command bytes and GPU pixels are not captured separately.

The heartbeat at **634728** precedes the copy/getter sequence: TaskThread six,
StaticR 4108, valid FST, coherent fiber/current/running `0x80347498`, active
renderer/frame, **5,988 FIFO writes**, **102 successful presents / zero
failures**, zero replay. EndDisplayList records 32 bytes in a 64-byte buffer
at `0x921032E0`, saved context enabled. These counters do not identify copied
pixels; elapsed time is not a performance comparison.

## Next bounded audit

Pinned `gx_transform.cpp` reads the saved guest-space viewport, not a
renderer-scaled native viewport. The existing Switch setter forwards to Aurora
but does not keep that six-float shadow. The getter also runs a frame-gated
MKW offscreen-screen flag sweep before checking a null output, then makes
best-effort big-endian float writes. Returning invented defaults or omitting
that known side effect would not preserve the pinned behavior.

The checked draw helper calls a projection/viewport setup immediately after
this getter. That setup also needs GXSetZScaleOffset `0x80173400`, which is a
static forecast, not an observed return/frontier. A bounded transform-state
candidate can audit both dependencies together. Other future calls and
argument families remain blocked until understood.
