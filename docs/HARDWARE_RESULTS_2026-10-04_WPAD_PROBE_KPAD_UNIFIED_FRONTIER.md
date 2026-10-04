# WPADProbe returned; KPAD unified status frontier — 2026-10-04

The fresh run accepts WPADProbe returning on channel 0 and reaches
**KPADGetUnifiedWpadStatus `0x8019812C`**, at dispatch 617560 /
107.369 seconds. The user confirms **black output followed by an error**.
Recognizable game pixels remain unproven.

## Candidate and verified reports

The Rendered Discovery NRO contains code `2941f1d`; launch HEAD `535699f`
adds validation documentation only. Fifteen local contracts, six WPADProbe
mutation checks, both full builds and five exact-code workflows / six jobs
pass. The NRO is 73,490,488 bytes, SHA-256
`82685e875a2a7dfcdb76ebe627e2dd82da8bac46fe79a0643f572e9fd660f712`, with
47 retained strong symbols and 18 scoped providers.

Discovery replied from the Switch at 19:43:35 UTC. Nxlink transferred
26,741,382 compressed bytes / 2,248 blocks with exit 0, from 19:43:47 to
**19:44:01 UTC**. No TCP preflight consumed the listener. The earlier
connection failures were before transfer and are not game crashes.

USB/MTP retrieval at **19:46:21 UTC** supplied **33 reports / 542,797 bytes**.
Every size, SHA-256, baseline difference, raw ZIP member byte and CRC was
independently verified. Thirteen reports changed and twenty match the
preceding PADRead run. Source timestamps are unavailable; identical files
cannot independently establish a fresh invocation. Private NROs, raw reports
and diagnostic archives remain excluded from publication.

## Accepted scope and new stop

The new WPADProbe report records `no-remote-pass`, channel 0, type pointer
`0x80398F40`, result -1, expected core type 0 and absent remote backend.
Discovery orders WPADProbe before KPADGetUnifiedWpadStatus, with the same LR
`0x8051EEC0`. Both native calls share dispatch count 617560. The later target
and restored caller arguments establish WPADProbe returned; a larger dispatch
count is not required between two native calls. The raw four-byte output is
not captured, and other WPAD channels remain outside hardware acceptance.

The new stop has r3=0 (channel), r4=`0x80398F50` (output), r5=1 (count),
stack `0x80398F38`, r6=7, r7/r8=0, r13=`0x8038CC00` and r2=`0x8038EFA0`.
The primary pinned `runtime/src/hle/input/kpad.cpp` override and public
RMCP01 symbol map identify `KPADGetUnifiedWpadStatus`. The stage
`RMCP01_WPAD_PROBE` is the last marker, not evidence of being stuck inside
WPADProbe.

PADRead again reports a connected port 0, a 48-byte status buffer at
`0x9096E010` and zero rumble mask. Later translated PADClampCircle2 reaches
that same buffer. Per-button behavior remains untested.

## Rendering and liveness limits

The earlier snapshots retain 3,937 FIFO writes, 168 GXBegin hits, 103 GXFlush
hits, 99 successful presents / zero failures and zero display-list replay
calls. Post-main snapshot 617479 and heartbeat 617265 precede the new stop.
They do not establish rendered content after WPADProbe. The watchdog has
103 ACTIVE samples and one recovered STALE sample, maximum interval 1,311 ms
and a final ACTIVE sample. Guest current/running thread `0x80347498` remains
coherent and the FST is structurally valid.

Scheduling changes dispatch/time totals; these are not performance comparisons.
KPAD unified status and subsequent calls, actual replay, recognizable images,
sustained execution, full input/audio behavior and performance remain open.
See the [WPADProbe implementation](WPAD_PROBE_2026-10-04.md) and
[preceding PADRead result](HARDWARE_RESULTS_2026-10-04_PAD_READ_WPAD_PROBE_FRONTIER.md).
