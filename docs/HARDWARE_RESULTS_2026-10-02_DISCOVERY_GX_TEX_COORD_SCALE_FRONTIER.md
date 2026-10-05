# Hardware result — texture matrices crossed / scale frontier (2026-10-02)

Code candidate `02010aaaa974581f49ed2dddfd02ebf0e2bc0e96` produced the
73,293,880-byte Rendered Discovery NRO, SHA-256
`e666413d2c76a2ef9a5a278a4feb4a1e94ae01632464b643a62f885905bb656e`.
Nxlink completed with exit 0 at 17:39:32 UTC (19:39:32 Europe/Paris).
USB/MTP retrieval at 17:43:49 UTC preserved 28 text reports, 526,625 bytes.
All manifest hashes/sizes and the full diagnostic ZIP CRC were verified.
The raw reports and analysis remain local under
`.deps/network-tests/gx-load-tex-mtx-imm/runs/20261002T174349Z`.

## Matrix bridge acceptance

| Fresh evidence | Dispatch | State |
| --- | --- | --- |
| Entry `0x802412C8` | 605332 | previous stage `RMCP01_GX_LOAD_TEX_OBJ`, map 7 |
| First `GXLoadTexMtxImm`, `0x80173234` | 605340 | matrix `0x802581C8`, id 30, type 0 |
| New direct frontier `0x80171180` | 605350 | `ScaleManually(0,0,0,0)`, stage `RMCP01_GX_SET_TEX_COORD_GEN2` |

The later scale frontier has LR `0x80241334`, the same matrix-call stack
pointer `0x80398FB8` and guest fiber `0x80347498`. The durable blocker and
last discovery entry agree on that state.

The generated local `func_802412C8` starts at its normal entry and executes
a ten-iteration matrix loop, ids `30,33,...,57`, type 0, before Gen2 coord 0
and then Scale coord 0. Reaching this exact scale site therefore establishes
that all ten matrix bridge calls and Gen2 coord 0 returned. The exact +10
instrumented dispatch delta from the first matrix to the scale frontier
matches the nine remaining matrices plus Gen2. The missing DIRECT call
records its discovery frontier without incrementing the dispatch counter.

This is proof from executed control flow, coherent state, a later distinct
frontier and the dispatch delta. Discovery stores only the first occurrence
of each target; it does not contain ten separate matrix records. No matrix
coefficients were captured or turned into fixtures. Other matrix types,
post-texture ids and arbitrary guest backing remain outside this hardware
acceptance. Their relevant host contracts are documented separately in
`GX_LOAD_TEX_MTX_IMM_2026-10-02.md`.

## New frontier and termination time

```text
kind                  : DIRECT
target                : 0x80171180
elapsed_ms            : 99513
dispatch count        : 605350
lr                    : 0x80241334
r3/r4/r5/r6           : 0 / 0 / 0 / 0
fast-track stage      : RMCP01_GX_SET_TEX_COORD_GEN2
action                : abort after durable blocker record
```

Pinned WiiCompiled maps `0x80171180` to GXSetTexCoordScaleManually.
The new elapsed-time instrumentation locates this boundary at 99.513 seconds
from the first translated dispatch, excluding nxlink transfer time. This
confirms the user's report that the NRO exits after more than a minute.
The report and audited dispatch path explain the known termination as an
intentional unsupported-call abort; no 60-second termination timer was found.
This does not measure when a system error dialog became visible.

The watchdog history contains 97 ACTIVE samples and no STALE samples, with
its last sample at 98,636 ms, dispatch 605171. No native exception report was
retrieved; that absence does not exclude every possible native failure.

The next boundary's exact pinned contract and its nearby Bias/Gen2 variations
are already audited in `GX_TEX_COORD_NEIGHBORS_2026-10-02.md`. Scale is now
observed. Bias and Gen2 coords 1..7 remain static forecasts, not hardware
crossings. No coordinate bridge is implemented by this result record.

## Health, freshness and visual limits

The changed durable snapshot at dispatch 605265 precedes the matrix bridge.
It records main reached, six TaskThread::run calls, coherent fiber/current/
running identities `0x80347498`, a valid 64,224-byte FST, initialized renderer
and active frame. It retains FIFO writes 1556, GXCopyDisp 99, successful
presentations 99 and failed presentations 0. These counters do not
independently measure the new matrix emissions or prove recognizable pixels.

Twelve reports differ from the IA8 baseline: DVD read, discovery targets,
dispatch blocker, texture wrap mode, heartbeat history and heartbeat,
OS message events and receive-message frontier, sleep events, last post-main
dispatch, SZS decode and thread events. Sixteen byte-identical reports are not
individually datable. Source MTP timestamps are unavailable. Fresh target,
state, dispatch delta and the newly added timing fields attribute the frontier
to this candidate rather than relying on those timestamps.

Static coverage is 142 native, 10,494 translated and 313 missing among 10,949
direct targets; 838 runtime-seen, 20 runtime-seen missing. This is not a count
of future blockers. A screen observation for this launch is still pending.
The previous run's black screen remains historical evidence; no recognizable
game image is established by the new diagnostic reports alone.
