# Hardware result — TEV colors to AlphaCompare (2026-10-03)

The [bounded color/table candidate](GX_TEV_COLOR_BATCH_2026-10-03.md), code
`1333b0e2c5b6695a2d8512ee1080041792308718`, produced the 73,343,032-byte
Rendered Discovery NRO, SHA-256
`a56be88113ff7c2cc20808111cf7d6c0e947b0c8955b2252737b974b28a9e0ad`.
All five GitHub workflows passed on that code and on documentation-only
revision `2bb36be56b0a7dfaa90ed4f529ff0af1a858c2fd`. The launch helper verified
the artifact, source hashes, dependency pins and preserved integration patch.

Nxlink completed with exit 0 at 2026-10-03 13:43:58.442698 UTC
(15:43:58 Europe/Paris). Retrieval at 13:53:12 UTC preserved 28 reports,
530,070 bytes: eleven changed against the accepted scalar baseline and
seventeen byte-identical. MTP source timestamps are unavailable; unchanged
reports cannot be independently dated. Manifest hashes/sizes and full ZIP CRC
were verified. Raw reports, coverage and checked analysis remain local under
`.deps/network-tests/gx-tev-color-batch/runs/20261003T135312Z/`.

The user again reported **a black screen and a crash**. Exact on-screen error
wording was not captured. The durable report records the intentional abort at
an unsupported translated DIRECT call. Recognizable game pixels remain unproven.

## Fresh progression and accepted scope

| First-hit entry | Dispatch | Captured arguments/context |
| --- | --- | --- |
| KColor `0x80171ED4` | 700066 | ID 0, RGBA pointer `0x80398FCC`, SwapMode stage |
| Color `0x80171E10` | 700076 | ID 0, RGBA pointer `0x80398FC8`, KColor stage |
| SwapModeTable `0x8017200C` | 700080 | `(0,0,1,2,3)`, Color stage |
| Later caller `0x80241530` | 700084 | LR `0x80240F9C`, restored r1 `0x80398FD8`, SwapModeTable stage |
| AlphaCompare `0x80172088` | 700091 | `(7,0,0,7,0)`, nested r1 `0x80398FC8`, BlendMode stage |

All entries retain r2 `0x8038EFA0`, r13 `0x8038CC00` and fiber
`0x80347498`. The three setter entries share LR `0x80240F98` and r1
`0x80398FB8`. Discovery and the durable blocker agree on AlphaCompare's
target, dispatch, arguments, stack and LR.

The checked generated `func_80241380` loops KColor IDs 0..3 and Color IDs
0..3, then calls SwapModeTable with `(0,0,1,2,3)`, `(1,0,0,0,3)`,
`(2,1,1,1,3)` and `(3,2,2,2,3)`. It restores the stack and returns to
`func_80240F68`, which then invokes `func_80241530`. That later coherent caller
and distinct AlphaCompare boundary establish return from **all twelve calls**:
four KColor, four Color and four SwapModeTable calls. Discovery records first
hits, not twelve individual returns. The first-KColor-to-caller delta is 18,
versus the callback-free forecast 12; its excess is compatible with VI polling,
but the exact callback count is not established.

The earlier sixteen-stage scalar loop, ten type-0 matrices, eight disabled
coordinate triples and documented IA8 path remain crossed. Actual RGBA bytes
are still absent from the reports; native byte forwarding is covered by the
host contracts. This console run does not accept alternate arguments, negative
guards, every native BP emission or visible pixels.

## Termination, timing and preceding state

AlphaCompare is **arrived at, not returned**. The DIRECT report records
dispatch 700091, elapsed_ms 362297, LR `0x80240F9C`, r1 `0x80398FC8`,
stage `RMCP01_GX_SET_BLEND_MODE`, and `abort after durable blocker record`.
Its preceding existing BlendMode call returned.

The watchdog has 101 samples: 99 ACTIVE and two transient STALE samples,
each followed by ACTIVE progress. There is a **256.581-second sampling gap**
between host_ms 60378 and 316959, with the same dispatch 46133 on both sides.
Its cause is unknown. The 362.297-second elapsed value must not be interpreted
as uninterrupted execution time, a persistent hang, or a performance comparison
with the previous run. A fixed three-minute wait did not cover this run's full
wall-clock duration; the user opened MTP after the program stopped.

The changed durable heartbeat at dispatch 699651 precedes the matrix,
coordinate and TEV loops. It records coherent scheduler/fiber state, 3826
StaticR dispatches, six TaskThread::run calls, a valid 64,224-byte FST at
`0x97DC0000`, initialized renderer and active frame. Its guest FIFO counter is
1555, with 99 CopyDisp calls, 99 successful presents and zero failures. These
preceding counters do not measure later native TEV commands or prove pixels.
The graphics report itself is byte-identical to the baseline. No native
exception report was retrieved; absence is not exhaustive proof.

## Next boundary

GXSetAlphaCompare `0x80172088` is the next observed missing function. The
pinned wrapper sets the existing host `g_alphaCompareValid` flag before native
forwarding, converts references to u8, and passes compare/operator enums.
A separate bridge must preserve that explicit flag effect and the native BP
0xF3 command, with guards and executable validation. Fog and later setters
remain forecasts outside this hardware acceptance.
