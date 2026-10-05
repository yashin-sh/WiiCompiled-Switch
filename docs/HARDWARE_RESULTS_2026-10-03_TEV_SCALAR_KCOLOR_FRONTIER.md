# Hardware result — TEV scalar loop to KColor (2026-10-03)

Integrated code `e76e8f38d7bbb8c7e5781f324c7eae8541dedbaa` produced the
73,310,264-byte Rendered Discovery NRO, SHA-256
`cc88a78caf2583332f277f3544bdc3013d45e8572630850a8b16dde57e701ffb`.
Its port sources match the locally validated six-setter candidate
`549ef801e1948b65c0ce47d9dbd6417c85d1f69d`; subsequent changes concern
documentation, repository instructions and lint base selection. All five
GitHub workflows passed on the integrated revision, including both build jobs.
The private build was revalidated with unchanged pins, integration patch and
artifact digest.

Nxlink completed with exit 0 at 2026-10-03 11:39:21.665507 UTC
(13:39:21 Europe/Paris). Retrieval at 11:42:57 UTC preserved 28 reports,
528,821 bytes. Eleven differ from the accepted audit baseline; seventeen are
byte-identical and cannot be independently dated. MTP source timestamps are
unavailable. Manifest hashes/sizes and full ZIP CRC were verified. Raw reports,
coverage, manifest and checked analysis remain local under
`.deps/network-tests/github-sync-2026-10-03/runs/20261003T114257Z/`.
The exact successful transfer, changed coherent discovery/blocker/heartbeat
cohort and new implemented stages attribute the result to this NRO.

The user reported **a black screen and an error message at the end**. The
exact on-screen error text was not captured. The durable report records the
intentional unsupported-call abort at KColor; it does not establish the wording
of the on-screen message. Recognizable Mario Kart Wii pixels remain unproven.

## Fresh entries and later frontier

| First-hit entry | Dispatch | Captured arguments |
| --- | --- | --- |
| Caller `0x802412C8` | 604877 | LR `0x80240F94`, r1 `0x80398FD8` |
| Texture matrix `0x80173234` | 604885 | pointer `0x802581C8`, ID 30, type 0 |
| Scale `0x80171180` | 604896 | `(0,0,0,0)`, LR `0x80241334` |
| Bias `0x801711FC` | 604897 | `(0,0,0)`, same LR |
| Caller `0x80241380` | 604925 | final coordinate r3=7, r8=125, restored r1 `0x80398FD8`, Bias stage |
| Direct `0x80171B58` | 604927 | stage 0 |
| ColorIn `0x80171CE0` | 604935 | `(0,15,15,15,15)` |
| ColorOp `0x80171D60` | 604936 | `(0,0,0,0,1,0)` |
| AlphaIn `0x80171D20` | 604937 | `(0,7,7,7,7)` |
| AlphaOp `0x80171DB8` | 604944 | `(0,0,0,0,1,0)` |
| SwapMode `0x80171FD0` | 604945 | `(0,0,0)` |
| KColor `0x80171ED4` | 605056 | ID 0, pointer `0x80398FCC`, SwapMode stage |

The TEV entries and blocker retain LR `0x80240F98`, r1 `0x80398FB8`,
r2 `0x8038EFA0`, r13 `0x8038CC00` and guest fiber `0x80347498`.
The last discovery entry and durable blocker agree on KColor and its arguments.
Pinned WiiCompiled/Aurora remains
`a135beb201042b20f390c6695ca6b26768820fb4`.

## Accepted executed scope

The checked generated caller `func_80241380` initializes stage count 1, then
loops through stage IDs 0..15, calling Direct, existing Order, ColorIn,
ColorOp, AlphaIn, AlphaOp and SwapMode in that order. Only after all sixteen
iterations does it prepare the color bytes and dispatch KColor ID 0 with
pointer r1+20. Fresh first hits, coherent state and that later distinct
boundary therefore establish return from **96 new setter calls plus 16
existing Order calls** on the documented default tuples. Discovery does not
contain 96 individual return records.

The Direct-to-KColor delta is 129, versus the callback-free forecast 111.
The additional dispatches are compatible with VI callback polling; first-hit
tracing does not determine their exact identity or number. The delta alone
is not the acceptance proof. The earlier ten texture-matrix calls and eight
Gen2/disabled-Scale/disabled-Bias triples retain their accepted executed scope.

This result does not hardware-validate alternate color/alpha enums,
comparison operations, noncanonical clamp values, every native BP command or
display-list decoding. Those retain the host contracts and pinned native
provider audit described in [the batch record](GX_TEV_SCALAR_BATCH_2026-10-03.md).

## Termination and health

KColor ID 0 is **arrived at, not returned**. The unsupported DIRECT report
records dispatch 605056, elapsed_ms 98265, pointer `0x80398FCC`, stage
`RMCP01_GX_SET_TEV_SWAP_MODE` and `abort after durable blocker record`.
The exit follows a missing function after 98.265 seconds from the first
translated dispatch; it does not establish a one-minute timer or persistent
scheduler hang. All 95 watchdog samples are ACTIVE, with no STALE sample.
No native-exception report was retrieved; absence is not exhaustive proof.

The changed durable snapshot at dispatch 604804 precedes the matrix,
coordinate and TEV loops. It retains six TaskThread::run calls, 3826 StaticR
dispatches, valid 64,224-byte FST at `0x97DC0000`, coherent scheduler/fiber
identities, initialized renderer and active frame. Guest FIFO writes remain
1556, GXCopyDisp calls 99, successful presents 99 and failures zero.
These preceding counters do not measure the later native TEV emissions or
prove visible game content. Native Aurora writes can bypass the instrumented
guest FIFO counter.

## Next boundary and limits

KColor `0x80171ED4` introduces a four-byte guest RGBA pointer read. ID 0 and
r1+20 are captured; the RGBA bytes are not. It remains a hard stop until its
ID-before-memory guard, complete readable range, exact byte copy and native
forwarding have a separate bounded implementation and validation.
GXSetTevColor `0x80171E10` and GXSetTevSwapModeTable `0x8017200C` remain
static later forecasts. No guest color data is assumed from this run.

SIZE_MAX rejection, Present(false), teardown exceptions and controlled
shutdown/error recovery were not exercised here. Input mapping, native audio,
recognizable pixels and playability remain open.
