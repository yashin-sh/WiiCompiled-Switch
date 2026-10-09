# Capture-disabled present-rate trial — 2026-10-09

The operator sees the initial Wiimote warning page, then a black screen with
PR #339 code `561424e`. Both capture controllers report `DISABLED` with fresh
run identifiers and the SD startup marker as their reason. All measured windows
report `readback_enabled=0`. No new PNG or FIFO capture is attributed to this run;
older capture files retained on SD belong to earlier trials.

## Measured presentation rate

Eight windows contain 93 completed presentations over 79.065 seconds, from the
first presentation to the last measured window. Their combined rate is about
1.18 presentations per second. Individual windows range from 0.234 to 3.652 Hz;
one spans 42.752 seconds for ten presentations. Counts and window durations
independently reproduce each rounded rate in the renderer log.

These are completed-present frequencies, including loading, translated guest
execution and stalls. They are not steady gameplay FPS. Disabling image/FIFO
capture alone does not make this observed startup smooth. The preceding trial
has no equivalent rate windows, so a numerical speedup cannot be established.

The renderer records 94 successful presents and zero present failures. The
watchdog records ongoing translated dispatch through roughly 85 seconds, and
the final sampled report has coherent main/FST/fiber state. The retrieved reports
contain no terminal blocker or exception. A last dispatch in GXFlush does not
establish that function as the cause of the subsequent black screen.

## Diagnostic SD cost and correction

Inspection finds that the post-main diagnostic rewrites a multi-kilobyte report
and calls `fsync` on every recurring scheduler/GX phase target. The current run
records thousands of such calls. This is an avoidable hot-path SD cost, but its
share of the observed slowdown has not yet been timed on hardware.

The correction retains the first sixteen post-main snapshots, then samples the
report at most once per second across all targets. In-memory counters continue
on every dispatch. The existing bounded startup traces and once-per-second
heartbeat remain; terminal blocker and exception writes bypass the sampling
budget. Zero ticks, a missing clock frequency and a clock rollback cannot turn
sampling into an unbounded write loop.

A host contract executes the real diagnostic translation unit under ASan/UBSan,
counts synchronization calls during a million repeated phase hits, checks
periodic snapshots and current counters, and verifies immediate terminal files.
The same contract is part of public CI. Hardware performance after this
correction remains pending, as does the cause of the later black screen.

## SD cleanup and evidence retention

At the user's request, 40 older project NRO copies were backed up and verified
before removal from SD. The current validated candidate and other applications
remain. Deployment receipts or two independent full USB reads establish backup
identity; recovery copies and the cleanup manifest stay private.

Executed-product identity, report retrieval checksums, raw diagnostics and private
NRO backups are excluded from public source. The earlier captured-pixel result
remains in the [preceding trial](HARDWARE_CAPTURE_CONTROL_2026-10-09.md).
