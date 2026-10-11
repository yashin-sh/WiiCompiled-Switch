# NANDSeek candidate launch: existing-file length frontier — 2026-10-11

The candidate `182b6e20325e30c414368b5bac6f9514a314bea5` passes its separate private rendered/Discovery/SDK
gate and all eight checks in the complete exact-HEAD rollup before
[PR #381](https://github.com/yashin-sh/WiiCompiled-Switch/pull/381) merges.
Complete SD size/SHA-256 readback, an independent capture-disabled marker read,
and verified backup/removal of the old project NRO all pass. The exact
74,469,432-byte candidate launches through Netloader with sender exit 0
at 2026-10-11T09:22:31.524056+00:00. Workspace `96dd167ae9ebc05e3d214a1760f56f327c7386f0` differs only in
public documentation. This launch reuses the marker verification at
2026-10-11T09:19:42.193786+00:00; fresh runtime statuses independently
confirm DISABLED afterwards. Binary products, raw reports, callers and game
data remain private.

The operator returns to USB/DBI and reports that the trial stopped; no new visual
description is supplied for this trial. Retrieve 42 reports /
467,723 bytes and verify every report through a second independent
USB copy: 22 changed, 20 retained.
The actual transmitted local binary and the complete current SD NRO match the
recorded size/SHA-256. No fresh NANDSeek status exists, and this run supplies no
accepted execution of the rewind. Retained NAND write/move statuses cannot be
used as new return witnesses.

The changed DIRECT terminal instead names NANDGetLength `0x8019BF4C`, dispatch
288491, elapsed 28381 ms, PC `0x8024373c`, LR
`0x8052c268`, SP `0x902303a8`. The file-info pointer is `0x902303e4` and the four-byte
length output is `0x902303e0`, immediately before the descriptor. Both are on
fiber `0x902304E0`. Checked parent `0x8052C20C` opens the configured path in
ordinary read mode 1, then calls helper `0x8052B3F8` to query its length and
compare it with caller input `0x002BC000`. The path itself and live descriptor
properties are not recorded here; do not guess a filename or substitute the
expected size for a real query. The [next bounded length bridge](NAND_GET_LENGTH_2026-10-11.md)
must prove ownership, host read mode, ordinary origin and complete disjoint
input/output spans before operating on the real stream.

Capture reports are freshly DISABLED with their individual run ids. The changed
texture status records an 832 × 456 load-pass, format 4. No Dawn uncaptured
error or exception report appears. Uncontrolled present windows yield about
4.04 Hz, which supplies no controlled performance comparison.
No retained image is attributed to this trial. Menus, controls, gameplay,
rewind execution and complete save/recovery acceptance remain unproven.
