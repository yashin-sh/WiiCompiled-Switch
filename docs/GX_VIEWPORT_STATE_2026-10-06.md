# Guest viewport and depth state — 2026-10-06

This bounded candidate addresses the [observed GXGetViewportv frontier](HARDWARE_RESULTS_2026-10-06_PIX_MODE_SYNC_VIEWPORT_FRONTIER.md)
`0x801733E0`, output `0x80397B10`, dispatch 635096. The checked Mii draw setup
also calls GXSetZScaleOffset `0x80173400`; that dependency is a static forecast,
not an observed hardware return. Both new boundaries remain hardware-unaccepted.

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
The devkitA64 rendered syntax gate includes the new bridge. Complete local
suite, synthetic build, exact-head GitHub workflows, private rendered build
and provider ownership evidence are required before deployment. Their results
will be recorded after completion; no hardware result is claimed here.

Unknown APIs and copy argument families remain blocked. This candidate does
not prove GPU completion, texture pixels or recognizable game images.
