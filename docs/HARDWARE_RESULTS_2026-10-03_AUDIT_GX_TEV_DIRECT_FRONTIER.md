# Hardware result — audit normal-path non-regression (2026-10-03)

Audit code candidate `b3484117f5923d72a04d860eecd702452157cdde` produced the
73,297,976-byte Rendered Discovery NRO, SHA-256
`7ecbc8a9fe1efb31697c2ee36d0b0b648a8e87d3fa7dda1fb9d262d6de5b7d09`.
Nxlink completed with exit 0 at 2026-10-03 09:25:19.409664 UTC
(11:25:19 Europe/Paris). USB/MTP retrieval at 09:32:30 UTC preserved
28 text reports, 528,058 bytes. Manifest hashes/sizes and the full ZIP CRC
were verified. Raw reports and analysis remain local under
`.deps/network-tests/audit-2026-10-03/runs/20261003T093230Z/`.

The user confirmed a black screen. This run accepts **normal-path
non-regression of the audit candidate**, not every corrected error path or
recognizable Mario Kart Wii pixels.

## Same executed path and frontier

| Fresh evidence | Dispatch | Captured state |
| --- | --- | --- |
| Entry `0x802412C8` | 608308 | LR `0x80240F94`, r1 `0x80398FD8` |
| First matrix `0x80173234` | 608316 | pointer `0x802581C8`, ID 30, type 0, r1 `0x80398FB8` |
| First Scale `0x80171180` | 608333 | `(0,0,0,0)`, LR `0x80241334`, stage Gen2 |
| First Bias `0x801711FC` | 608346 | `(0,0,0)`, same LR/stack, stage ScaleManually |
| Later caller `0x80241380` | 608374 | LR `0x80240F98`, restored r1 `0x80398FD8`, r3=7, r4..r7=0, r8=125, stage Bias |
| DIRECT `0x80171B58` | 608381 | TEV stage ID 0, LR `0x80240F98`, r1 `0x80398FB8`, stage NumTevStages |

The entries retain r2 `0x8038EFA0`, r13 `0x8038CC00` and guest fiber
`0x80347498`. The last discovery entry and durable blocker agree.
Pinned WiiCompiled/Aurora remains
`a135beb201042b20f390c6695ca6b26768820fb4`.

The restored caller state again establishes return from all eight
Gen2(c,1,4,60,0,125), Scale(c,0,0,0) and Bias(c,0,0) triples, c=0..7,
following the previously accepted ten type-0 matrix calls. The existing
GXSetNumTevStages(1) bridge returns before the distinct unsupported Direct
frontier. This preserves the accepted coordinate path from
[the earlier coordinate run](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md).
Enabled Scale/Bias branches and arbitrary Scale sizes remain host-only.

Scale-to-caller is +41 dispatches, versus the callback-free +23, and
caller-to-frontier is +7, versus +1. These extra dispatches are compatible
with VI polling and callback work while the interrupted CpuContext is
restored. First-hit tracing records entries, not every occurrence or return;
it does not establish an exact repeated callback count.

## Termination and health

```text
kind                  : DIRECT
target                : 0x80171B58
elapsed_ms            : 107925
dispatch count        : 608381
lr                    : 0x80240F98
r1                    : 0x80398FB8
r3                    : 0
r8                    : 125
fast-track stage      : RMCP01_GX_SET_NUM_TEV_STAGES
action                : abort after durable blocker record
```

GXSetTevDirect stage 0 is arrived at, **not returned**. Its unchanged
unsupported-call abort is the frontier, after 107.925 seconds from the first
translated dispatch, excluding transfer time. The watchdog records 101
ACTIVE samples and one temporary STALE sample followed by renewed progress;
this does not establish a persistent hang.

The changed durable snapshot at dispatch 608109 precedes the matrix and
coordinate loops. It retains six TaskThread::run calls, 3826 StaticR
dispatches, valid 64,224-byte FST, coherent current/running/fiber identities,
initialized renderer and active frame. FIFO writes are 1556, GXCopyDisp calls
99, successful presents 99 and failures zero. These counters preserve the
earlier baseline but do not measure later native GX emissions or prove game
pixels. No native-exception report was retrieved; absence is not exhaustive
failure proof.

Twelve reports differ from the coordinate baseline, including blocker,
discovery, heartbeat/history and last dispatch. Sixteen are byte-identical
and cannot be independently dated; source MTP timestamps are unavailable.
The exact successful audit transfer and changed coherent report cohort
attribute this run to NRO `7ecbc8a9...`, rather than reusing the earlier
coordinate result for audit acceptance.

## Acceptance limits and next candidate

This console run did not exercise SIZE_MAX range/stack rejection,
Present(false) cleanup, teardown exceptions or controlled shutdown/error
recovery. Their local contracts and remaining limits are documented in
[PORT_AUDIT_2026-10-03.md](PORT_AUDIT_2026-10-03.md). It does not extend the
port's compatibility domains or prove the absence of unrelated defects.

The next [bounded TEV scalar batch](GX_TEV_SCALAR_BATCH_2026-10-03.md) targets
six setters on legal SDK domains. It remains a candidate until its executable
contracts, exact-candidate workflow gates, private build and fresh console
progression are established. The following KColor guest-pointer boundary is
outside that batch.
