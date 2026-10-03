# Hardware result — AlphaCompare returned, Fog captured (2026-10-03)

Integrated code `1a8c092fbc7f1b6c4f9f0562a5b5d9a295f406eb` produced the
73,355,320-byte Rendered Discovery NRO, SHA-256
`7032c756f4f0872334aea0a4421a8633e8d76d9ed1c004cf2d59fafa87b5b310`.
The [AlphaCompare build record](GX_ALPHA_COMPARE_2026-10-03.md) documents its
host contracts, five successful workflows, private build, 27 symbols and
unique native/flag ownership. All five workflows also passed on the subsequent
documentation-only revision `046c23660a1f9a36cda43b940ca19c8289f302da`.

The launch helper revalidated sources, dependency pins, the existing integration
patch and exact artifact digest. UDP discovery and ping identified the ready
Switch at `192.168.1.194`; no separate TCP probe consumed a netloader connection.
Nxlink completed with exit 0 at **2026-10-03 17:50:16.808723 UTC**
(19:50:16 Europe/Paris).

USB/MTP retrieval at 17:55:15 UTC preserved 28 reports, 529,931 bytes.
Twelve differ from the accepted color/table baseline; sixteen are byte-identical
and cannot be independently dated. MTP source timestamps are unavailable.
Manifest sizes/hashes and full ZIP CRC were verified. Raw reports, coverage,
manifest and checked analysis remain local under
`.deps/network-tests/gx-alpha-compare/runs/20261003T175515Z/`.
The changed discovery/blocker/heartbeat cohort and the newly implemented Fog
diagnostic attribute this progression to the successfully transferred NRO.

**Visual output and exact on-screen error wording have not yet been confirmed
by the user for this run.** Previous black-screen observations are historical;
they do not establish this run's output. The durable report records an
intentional unsupported-DIRECT abort. Recognizable game pixels remain unproven.

## Fresh entries and accepted return

| First-hit entry | Dispatch | Captured arguments/context |
| --- | --- | --- |
| KColor `0x80171ED4` | 603934 | ID 0, pointer `0x80398FCC`, SwapMode stage |
| Color `0x80171E10` | 603938 | ID 0, pointer `0x80398FC8`, KColor stage |
| SwapModeTable `0x8017200C` | 603948 | `(0,0,1,2,3)`, Color stage |
| Later caller `0x80241530` | 603952 | LR `0x80240F9C`, restored r1 `0x80398FD8`, SwapModeTable stage |
| AlphaCompare `0x80172088` | 603954 | `(7,0,0,7,0)`, r1 `0x80398FC8`, BlendMode stage |
| Fog `0x801722CC` | 603961 | type 0, color pointer `0x80398FD0`, ZMode stage |

AlphaCompare and Fog share LR `0x80240F9C`, r1 `0x80398FC8`,
r2 `0x8038EFA0`, r13 `0x8038CC00` and guest fiber `0x80347498`.
The durable blocker agrees with Fog's first-hit target, dispatch and context.

The checked generated caller `func_80241530` invokes existing BlendMode, then
AlphaCompare `(7,0,0,7,0)`, existing ZMode `(1,3,1)`, and Fog after loading its
four float parameters and copying its guest color. The later distinct Fog
entry with ZMode stage and coherent state therefore establishes **AlphaCompare
returned on the observed tuple**, followed by return from the intervening
existing ZMode call. Discovery provides first hits rather than an individual
return record. Its AlphaCompare-to-Fog delta is seven, versus the callback-free
forecast two; excess dispatches are compatible with VI polling but their exact
identity/count is not established.

The twelve color/table calls and earlier documented scalar, matrix, coordinate
and IA8 path remain crossed. Alternate AlphaCompare tuples, wide references,
refusal branches, flag ordering and BP ownership retain the host/provider
evidence; this run has no separate console flag trace or native BP decode proof.

## Captured next boundary

Fog is **arrived at, not returned**. The unsupported DIRECT report records
dispatch 603961, elapsed_ms 99156, stage `RMCP01_GX_SET_Z_MODE`, type 0,
pointer `0x80398FD0`, and `abort after durable blocker record`.
The new diagnostic resolves the complete four-byte guest range successfully
and captures RGBA **255,255,255,255**. These are Fog's measured bytes; they
are not a retrospective measurement of earlier TEV colors.

| Parameter | Captured f64 bits | Value | Pinned float conversion: f32 bits |
| --- | --- | --- | --- |
| f1 / startZ | `0000000000000000` | 0 | `00000000` |
| f2 / endZ | `3FF0000000000000` | 1 | `3F800000` |
| f3 / nearZ | `3FB99999A0000000` | 0.10000000149011612 | `3DCCCCCD` |
| f4 / farZ | `3FF0000000000000` | 1 | `3F800000` |

The report copies f64 bits without narrowing or executing Fog. The f32 column
is the checked IEEE conversion used by the pinned native wrapper, whose float
arguments come from f1..f4 `.d`. It is not a separate native-call observation.
These actual values now support a bounded next Fog audit without a further
diagnostic-only console build. Native Fog, its floating-point normalization,
guest-color read and later translated/stateful work remain outside acceptance.

## Health and limits

All 96 watchdog samples are ACTIVE; the maximum interval is 1.304 seconds.
No STALE sample or long sampling gap appears in this run. Its 99.156-second
elapsed value is not a representative-scene performance measurement. No native
exception report was retrieved; absence is not exhaustive proof.

The changed durable heartbeat at dispatch 603545 precedes the matrix,
coordinate and TEV loops. It records coherent scheduler/fiber state, 3826
StaticR dispatches, six TaskThread::run calls, valid 64,224-byte FST at
`0x97DC0000`, initialized renderer and active frame. Its guest FIFO counter is
1556, with 99 CopyDisp calls, 99 successful presents and zero failures. These
preceding counters do not measure later native state commands or prove pixels.
The graphics report itself is byte-identical to the baseline.
