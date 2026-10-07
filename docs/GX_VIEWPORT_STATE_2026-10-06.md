# Guest viewport and depth state — 2026-10-06

This bounded candidate addresses the [observed GXGetViewportv frontier](HARDWARE_RESULTS_2026-10-06_PIX_MODE_SYNC_VIEWPORT_FRONTIER.md)
`0x801733E0`, output `0x80397B10`, dispatch 635096. The checked Mii draw setup
also calls GXSetZScaleOffset `0x80173400`; that dependency was a static forecast before the 2026-10-07 run. The [2026-10-07 run](HARDWARE_RESULTS_2026-10-07_VIEWPORT_MII_I4_LOAD_FRONTIER.md)
accepts both observed returns through the later guarded Mii texture load.

## Preserved behavior

The existing `GXSetViewport` bridge stores all six narrowed guest-space floats
before calling Aurora. Its initial shadow is `{0,0,1,1,0,1}`, matching pinned
WiiCompiled. The getter returns this shadow, avoiding renderer coordinate
mapping. Neither bridge clobbers the PPC context.

Before testing a null output, the getter performs the pinned frame-gated MKW
screen-list sweep. The bounded 64-node traversal checks screen membership and
vtable identity. Onscreen slots require framebuffer-canvas flag bit 3;
offscreen slots do not. Eligible screens gain keep-frustum-scale bit 6.
Repeated reads, the two frame tokens and their assignment before traversal
preserve the pinned exception/retry order. Unexpected sweep failure is a
durable diagnostic stop.

Six guest floats are written independently. Invalid fields log an access
violation while subsequent fields are attempted, and unsigned address wrapping
is preserved. `Memory::WriteFloat32` now uses the pinned Gekko/Broadway bit-level
`stfs` conversion, rather than a host rounding cast, and the existing checked
big-endian 32-bit store. This API does not add a new MMIO policy.

Depth setup narrows f1/f2, calls native `GXSetZScaleOffset` first, then best-effort
mirrors GXData at offsets `0x55C`, `0x560` and `0x5FC`. A late mirror fault keeps
preceding writes. Aurora stores scale/offset, emits two XF writes (six FIFO
operations), and clears `bpSent` afterward. Native failure stops durably.
Headless execution refuses both new operations explicitly; the established
headless viewport setter retains its previous behavior.

## Validation scope

The new ASan/UBSan contracts cover 65,536 setter/getter round trips, all 2,048
IEEE double exponent classes with signs, fractions and unaligned guest stores,
partial output mappings, screen membership/flags, once-per-frame side effects,
null outputs, finite list walks, wrapping, mirror partial writes and diagnosed
failures. A separate executable extracts the actual pinned Aurora depth body
and verifies FIFO widths/values, state and flag order in 108 cases. No game
product or copied proprietary function is published in the fixtures.

Public CI runs the new suite and retains synthetic probes for both addresses.
The devkitA64 rendered syntax gate compiles all 65 rendered sources. All 22
local suites and six compiled rejected mutants pass, as do the complete
synthetic ELF/NRO and all five actual code-head workflows / six jobs. Contract
counts and both retained synthetic bridges/probe are checked in CI logs.

Validated code is `cc0d633417a58e3c53847598d65a6ccc12e8f2e8`. The immutable-image,
network-disabled private rendered build exits 0, **20:08:07–20:36:14 UTC**.
All tracked build inputs are unchanged during that build. **69 required strong
functions** and **46 scoped unique providers** are verified over **235 host
objects/archives, 19 container Rust archives and seven named libraries**.
This includes the viewport shadow, frame counter and checked float writer;
provider ownership does not prove runtime pixels or every linked symbol.

The exact NRO is **73,621,560 bytes**, SHA-256
`73eb2313764fd89c8f777106b154ad7f6f80437161a623b01f68f94e8e905a16`.
USB/MTP deployment at **20:39:07–20:39:17 UTC** verifies every byte by complete
SD readback, at `sdmc:/switch/WiiCompiled-Switch-gx-viewport-state-rendered-discovery.nro`.
Original upstream patch bytes and nanosecond mtimes remain unchanged. Private
NROs, generated game products and raw archives remain excluded from GitHub.

All five publication-head workflows / six jobs pass on `c209b1f`.
[PR #319](https://github.com/yashin-sh/WiiCompiled-Switch/pull/319) merges as
`378267f` at 2026-10-06 20:56:08 UTC; publication changes Markdown only.

## Netloader launch — 2026-10-07

Direct nxlink starts at **07:52:41 UTC** and exits **0 at 07:53:15 UTC**,
sending **26,799,100 compressed bytes / 2,253 blocks (36.40%)** of that exact
validated NRO. Its SHA-256, dependency pins, upstream patch and all non-Markdown
candidate source hashes are rechecked before transfer. Launch revision is
`c209b1fb03d0d91df04697346ab56c6095966b96`.

The user reports **a black screen with the test still running**. This is an
intermediate visual observation. Fresh USB/MTP reports now establish getter
and depth return before the distinct guarded I4 texture load. Raw guest
outputs, final visible outcome and recognizable pixels remain unproven.
The transfer establishes the launch transaction; it does not establish that
these native calls returned or that the GPU produced game pixels.

Unknown APIs and copy argument families remain blocked. This candidate does
not prove GPU completion, texture pixels or recognizable game images.
