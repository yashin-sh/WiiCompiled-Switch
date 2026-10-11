# NANDMove returned; NANDSeek frontier — 2026-10-11

The exact NANDMove candidate `57a2ee079afb565fcbfaf393e67a3800a68b1aa5`
launches via nxlink with exit 0 at 10:38:33 CEST. The operator reports the
Wiimote screen, then black output and an error, and returns to USB/DBI.
Code PR #378 and SD-receipt PR #379 previously merge with all eight exact-HEAD
checks successful and the separate private rendered gate passed. Launch-receipt
PR #380 also merges after all eight checks succeed.

Retrieve 42 reports / 853,624 bytes and verify them through a second independent
USB copy: 19 changed, 23 retained. Both the actual transmitted local binary and
the complete current SD NRO match the recorded size and SHA-256. The current
MTP mount differs from the prior deployment address; verification uses the
fresh report mount, preserving the original receipt. Raw diagnostics, game
payloads, generated callers, NROs and readback receipts remain private.

The new NANDMove status records `move-pass`, result 0, source
`0x802581A3`, directory `0x90230370`, and complete matching live paths including
both terminators: `/tmp/banner.bin` and
`/title/00010004/524d4350/data`. This proves the relocated stack pointer is
admitted by live content rather than a fixed-address substitute. Entry dispatch
655143 is followed by save-worker helper `0x8052B4D4` at 655148, on fiber
`0x902304E0` with stage `RMCP01_NAND_MOVE`. Checked caller `0x8023AA78` accepts
move results 0 or -6; the fresh status selects 0 here. Two independent USB
reads confirm the destination banner contains 29,344 bytes, and listing the
virtual temporary directory verifies that the source entry is absent. This
accepts the observed bounded move and its filesystem effect. It supplies no
comparison with the original guest buffer or complete save/recovery acceptance.

The creation status is retained from the baseline and is not a fresh individual
creation-result witness. The write status is changed and records the complete
29,344-byte result. The checked write/close/home-query branches lead to the
observed move, as in the prior trial.

The new DIRECT terminal is NANDSeek `0x8019B964`, dispatch 655148, elapsed
56,419 ms, PC `0x8024373C`, LR `0x8052C790`, SP `0x90230378`.
Its r3 file-info pointer is `0x902303C0`; r4 offset and r5 origin are both 0.
Checked parent `0x8052C68C` opens an ordinary mode-2 handle and calls helper
`0x8052B4D4` with a 10,240-byte first chunk and offset 0. The helper attempts
this seek before its write branch and expects the resulting position to equal
the requested offset. Live descriptor ownership, mode, open flag and stream
position are not captured at this frontier. The
[bounded rewind correction](NAND_SEEK_2026-10-11.md) must validate those live
handle properties before performing the actual seek. The next write or later
seek offsets are not accepted by this report.

Capture statuses are freshly DISABLED with run id `1791709057622419200`;
the pre-launch marker expectation is now independently supported by these
runtime reports. The texture load status is also changed and records a
1024-square IA4/clamp load-pass. No Dawn uncaptured error or exception report
appears. Uncontrolled present windows total 102 frames / 48,770 ms, about
2.09 Hz; this is not a controlled performance comparison. No retained image
is attributed to this run. Black output/error is the current operator
observation; menus, controls, gameplay and fresh later pixels remain unproven.
