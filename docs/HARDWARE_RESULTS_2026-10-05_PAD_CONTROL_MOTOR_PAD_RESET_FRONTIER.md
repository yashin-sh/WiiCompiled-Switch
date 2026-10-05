# PADControlMotor returned; PADReset frontier — 2026-10-05

The validated [PADControlMotor candidate](PAD_CONTROL_MOTOR_2026-10-05.md)
returns on the observed channel 0 / STOP_HARD command 2. The next DIRECT stop
is PADReset `0x801AF0DC`, mask `0x70000000`, dispatch **619288 / 112.024 seconds**.
The current visual observation is pending; recognizable game pixels are not
established by these diagnostics.

## Candidate and retrieval binding

The private Rendered Discovery NRO is 73,494,584 bytes, SHA-256
`b4cb13ebb58a6cf7a1a567e5ed89ce73df0f7bdf905cc266fa7b6856d8a1feda`, code
`1cff562ad888f917a2ffd9c586e5e3cab47cb765`. Launch revision `0d230ed` changes
only Markdown and is included in merged PR #313. All non-Markdown candidate
hashes, dependency pins, original upstream patch and the NRO bytes/hash were
reverified before transfer. Direct nxlink exits 0 at **2026-10-05 17:13:07 UTC**,
sending 26,743,203 compressed bytes / 2,247 blocks. UDP discovery had no reply;
no TCP preflight was used.

USB/MTP retrieval at **17:16:45 UTC** copies **35 reports / 544,876 bytes**.
Every local report is checked against its manifest size/SHA-256 and the previous
[KPAD-run baseline](HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md).
Seven reports differ; twenty-eight are byte-identical. Raw ZIP members match
the copied bytes and ZIP CRC checks pass. A second direct SD read independently
confirms the small motor report. MTP source timestamps are unavailable; the
binding uses the verified transfer and newly recorded motor/later-blocker
sequence. No runtime build ID or raw guest output was captured. Private NROs,
raw reports and archives remain excluded from Git.

## Native return and new blocker

Discovery records first motor entry at dispatch **619267**, channel 0,
command 2, LR `0x8051EEEC`, stack `0x80398FA8`, coherent guest fiber
`0x80347498`. The new SD diagnostic records `no-actuator-pass`, channel 0,
command 2, rumble mask 0 and absent actuator. The later durable DIRECT blocker
is a different target, `0x801AF0DC`, at dispatch **619288**, stage
`RMCP01_PAD_CONTROL_MOTOR`. Together, entry, native report and later progression
establish the observed motor return; this is not proof of physical rumble or
other channels/commands.

PADReset captures:

```text
target            = 0x801AF0DC
mask / r3         = 0x70000000
r4                = 0x00000007
LR                = 0x80523848
r1                = 0x80399058
r2 / r13          = 0x8038EFA0 / 0x8038CC00
dispatch          = 619288
elapsed_ms        = 112024
stage             = RMCP01_PAD_CONTROL_MOTOR
action            = abort after durable blocker record
```

Pinned WiiCompiled `runtime/src/hle/input/pad.cpp` identifies this target as
`PAD__Reset_HLE(uint32_t mask)`, returning `PADReset(mask) ? 1 : 0`.
Pinned Aurora's PADReset ignores the mask and returns true without state changes.
PAL decomp symbols independently name PADReset at the same address. LR is in
`System::KPadDirector::calcControllers(bool)` (`0x805237E8..0x805238EF`);
this is function-family attribution, without a verified exact callsite claim.
The mask and r4 are captured register values, not an assertion that PADReset
has a second argument. Reset has arrived and has not returned in this run.

## Invariants and limits

The preceding heartbeat at dispatch **618833** records 3,937 FIFO writes,
99 successful presents / 0 failures, zero display-list replay calls, six
TaskThread hits, valid FST and identical guest fiber/current/running identities
`0x80347498`. These are earlier counters, not motor/reset rendering effects.
The watchdog has **106 ACTIVE samples and one recovered STALE sample**, with
maximum inter-sample interval 2,055 ms; later ACTIVE progression excludes a
persistent stall in this recorded path. Elapsed time is measured from the first
translated dispatch and is not a performance comparison.

Unchanged PADRead, WPADProbe and KPAD reports can be retained files. This run
accepts only the newly established motor `(0,2)` return. PADReset return,
physical controller reset/rumble, larger KPAD counts, per-button/multiplayer/
Wiimote behavior, audio, recognizable pixels and sustained gameplay remain open.
