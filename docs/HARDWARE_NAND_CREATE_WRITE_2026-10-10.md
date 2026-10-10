# NANDCreate returned; NANDWrite frontier — 2026-10-10

The corrected NANDCreate candidate `86b2ce5` launches via nxlink with exit 0
at 2026-10-10 19:39:47 CEST: 27,199,850 compressed bytes / 2,287 blocks.
Code PR #372 and deployment-doc PR #373 merge after all eight checks on
each exact HEAD succeed; the separate source-matched private rendered gate
also passes. Launch-receipt PR #374 follows with its own eight successful
checks. Complete SD readback verifies 74,436,664 bytes and SHA-256
`0a879f851099ab32e8a5024f6976dfe6f4fad9422c5162fd7e19968a772653b9`.
The previous NANDCheck NRO is backed up, size/SHA verified and removed,
leaving only the current project candidate.

After the operator returns to USB/DBI, 40 reports / 851,403 bytes are
retrieved and independently verified through a second USB copy. Ten
reports are changed and 30 retained. The entire SD NRO is also reread.
No current screen details are supplied; the user only reports readiness.
Raw diagnostics, game payloads, local callers and cleanup receipts remain
private.

The fresh creation status records `create-pass`, result 0, readable path
`0x802581A3`, permission `0x30`, attributes 0 and all 16 live bytes matching
`/tmp/banner.bin` including the terminator. The actual runtime guard verifies
those bytes before accessing virtual NAND. Entry dispatch 655898 is followed
by helper `0x8023AC3C` at 655899 and a distinct terminal at 655900, on fiber
`0x902304E0` with stage `RMCP01_NAND_CREATE`. The checked creation caller
reaches that helper only after result 0 or -6. Its fresh post-result status
selects 0 here. This accepts the observed bounded NANDCreate return. A separate complete USB
readback confirms the virtual temporary file exists and is empty before the
write entry is implemented.

The checked helper opens the same temporary path in mode 2, using a stack
file-info object, then reaches its write branch only after NANDOpen returns
0. The new DIRECT stop is NANDWrite `0x8019B884` at 58,196 ms: r3 file-info
`0x902302D0`, r4 buffer `0x80AE9540`, r5 length `0x72A0` (29,344), LR
`0x8023AAC8`, SP `0x902302C8`, same fiber. Live fd, handle state and buffer
contents are not captured in this report. The [bounded write bridge](NAND_WRITE_2026-10-10.md)
validates those live ranges and the actual owned host handle, path, mode,
open flag and initial position before writing. It does not guess payloads
from original static data or substitute a successful byte count.

Captures are freshly disabled. The texture-load report is unchanged from
the baseline and is not a fresh individual load witness. No Dawn uncaptured
error appears. Five uncontrolled windows total 102 frames / 50,966 ms,
about 2.00 Hz; this is not a controlled performance comparison. Older
images are not attributed to this run. Fresh pixels, menus, controls,
gameplay and steady performance remain unproven.
