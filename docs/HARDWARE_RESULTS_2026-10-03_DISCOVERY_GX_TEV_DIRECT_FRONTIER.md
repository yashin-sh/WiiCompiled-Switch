# Hardware result — coordinate batch crossed / TEV Direct frontier (2026-10-03)

Code candidate `91a4a01b8e316f9010e9d31754e279065772f9f3` produced the
73,297,976-byte Rendered Discovery NRO, SHA-256
`64ba837720f4e37cbd127c37a0e9bde6dc146ed229a92c8697b9c531a8984d08`.
The private build completed at 2026-10-02 22:56:35 UTC; nxlink completed with
exit 0 at 23:09:38 UTC (01:09:38 on October 3, Europe/Paris).
USB/MTP retrieval at 2026-10-03 09:03:40 UTC preserved 28 text reports,
526,932 bytes. All manifest hashes/sizes and the full diagnostic ZIP CRC
were verified. Raw reports and analysis remain local under
`.deps/network-tests/gx-tex-coord-batch/runs/20261003T090340Z/`.

The user confirmed a black screen for this coordinate launch. No recognizable
Mario Kart Wii image is established.

## Coordinate-loop acceptance

| Fresh evidence | Dispatch | Captured state |
| --- | --- | --- |
| Entry `0x802412C8` | 605560 | LR `0x80240F94`, r1 `0x80398FD8`, previous texture-map ID 7 |
| First matrix `0x80173234` | 605568 | r1 `0x80398FB8`, pointer `0x802581C8`, ID 30, type 0 |
| First Scale `0x80171180` | 605585 | `(0,0,0,0)`, LR `0x80241334`, stage `RMCP01_GX_SET_TEX_COORD_GEN2` |
| First Bias `0x801711FC` | 605592 | `(0,0,0)`, same LR/stack, stage `RMCP01_GX_SET_TEX_COORD_SCALE_MANUALLY` |
| Later caller `0x80241380` | 605620 | LR `0x80240F98`, restored r1 `0x80398FD8`, r3=7, r4..r7=0, r8=125, stage `RMCP01_GX_SET_TEX_COORD_BIAS` |
| New DIRECT `0x80171B58` | 605633 | stage ID 0, LR `0x80240F98`, r1 `0x80398FB8`, stage `RMCP01_GX_SET_NUM_TEV_STAGES` |

All six entries retain guest fiber `0x80347498`, r2 `0x8038EFA0` and
r13 `0x8038CC00`. The final discovery entry agrees with the durable blocker.

At pinned WiiCompiled/Aurora
`a135beb201042b20f390c6695ca6b26768820fb4`, the local generated
`func_802412C8` executes ten type-0 texture-matrix loads, then an
eight-iteration loop. For every coordinate c=0..7 it calls, in order:

```text
Gen2(c,1,4,60,0,125)
Scale(c,0,0,0)
Bias(c,0,0)
```

Only after all eight triples return does the function restore its stack and
return to `func_80240F68`, which calls `0x80241380` with LR `0x80240F98`.
The captured restored stack, final coordinate 7, remaining argument registers,
Bias stage and same fiber match this exact path. The later TEV frontier then
establishes continued progression and return from the existing
GXSetNumTevStages(1) bridge.

This accepts **all eight Gen2 tuples above, all eight disabled Scale calls and
all eight disabled Bias calls**. It also preserves progression through the
previously accepted matrix path. Discovery records only the first occurrence
of each target, before invocation. It does not provide 24 individual return
records; Gen2 had already appeared earlier in the run. Acceptance is an
executed-control-flow inference supported by coherent captured state and a
distinct later durable frontier.

Scale-to-later-caller is +35 dispatches rather than the no-callback prediction
of +23. Later-caller-to-TEV-frontier is +13 rather than +1. These extra +12
increments in each interval are compatible with VI polling: known dispatches
poll due retraces before recording their own entry, and callback work also
increments the counter. The VI service restores the complete interrupted
CpuContext afterward. First-hit tracing does not enumerate those repeated
callbacks, so this report does not claim an exact observed callback count.

Enabled Scale/Bias branches and arbitrary Scale sizes still have host contracts
only. Native return also does not prove every best-effort guest-mirror write
completed; no GXData memory dump was captured. The bounded contract and local
tests are recorded in [GX_TEX_COORD_BATCH_2026-10-03.md](GX_TEX_COORD_BATCH_2026-10-03.md).

## New frontier and termination

```text
kind                  : DIRECT
target                : 0x80171B58
elapsed_ms            : 100205
dispatch count        : 605633
lr                    : 0x80240F98
r1                    : 0x80398FB8
r3                    : 0
r4/r5/r6/r7           : 0 / 0 / 0 / 0
r8                    : 125
fast-track stage      : RMCP01_GX_SET_NUM_TEV_STAGES
action                : abort after durable blocker record
```

Pinned WiiCompiled maps this target to GXSetTevDirect, with r3 as its TEV
stage ID. Stage 0 is now observed, not returned. The other TEV calls remain
static forecasts. See [the TEV contract audit](GX_TEV_NEIGHBORS_2026-10-03.md).
The 100.205 seconds measure host time from the first translated dispatch,
excluding transfer time; the recorded exit remains an intentional unsupported
DIRECT abort.

The watchdog recorded a one-second STALE sample at 94,779 ms, followed by
ACTIVE at 95,808 ms and later progression. This is a recovered interval,
not evidence of a persistent hang. No native-exception report was retrieved;
that absence does not exclude every possible native failure.

## Freshness, graphics and separate audit artifact

Twelve of the 28 reports differ from the preceding matrix baseline, including
the discovery trace, blocker, heartbeat/history and durable last dispatch.
The 16 byte-identical reports are not independently datable; source MTP
timestamps are unavailable. New Scale/Bias entries, the later caller and the
distinct TEV blocker attribute this progression to the coordinate candidate.

The changed durable snapshot at dispatch 605367 precedes the matrix and
coordinate loops. It records six TaskThread::run calls, 3826 StaticR
dispatches, valid 64,224-byte FST, coherent current/running/fiber identities,
initialized renderer and active frame. Its 1556 FIFO writes, 99 GXCopyDisp
calls, 99 successful presents and zero failures do not independently measure
later native GX emissions. The confirmed black screen remains the visual
result of this run.

The separate audit code candidate `b3484117f5923d72a04d860eecd702452157cdde`
produced another 73,297,976-byte NRO, SHA-256
`7ecbc8a9fe1efb31697c2ee36d0b0b648a8e87d3fa7dda1fb9d262d6de5b7d09`,
at 2026-10-03 00:32:22 UTC. It had not been launched when these reports were
retrieved. This coordinate result does not validate that audit artifact's
runtime changes on hardware.
