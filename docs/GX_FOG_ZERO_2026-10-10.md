# Exact all-zero Fog tuple — 2026-10-10

The [fresh IA4 trial](HARDWARE_IA4_FOG_ZERO_2026-10-10.md) returns from a
retained IA4/repeat load, then stops at `GXSetFog` `0x801722CC`, type 0,
parameters `(0,0,0,0)`, transparent black. The bridge already admits the
initial `(0,1,0.1,1)` and Mii `(1,1,0,0)` tuples. Add only this third exact
positive-zero f64 tuple; other types, signed zeros, hybrids and unknown
parameters retain the diagnosed hard stop before memory lookup or narrowing.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` forwards r3,
f1..f4 `.d` narrowed to float, and the four-byte RGBA color at r4. Original
wrapper, native Fog code and register macro match the pin byte-for-byte.
The perspective branch uses A=0, B=0.5, C=0 when either range is zero, so
this tuple avoids division and normalizes safely. Output is `EE000000`,
`EF40000F`, `F0000001`, `F1000000`, then `F2rrggbb`, with `bpSent=1`.
Alpha is forwarded but is absent from FOGCLR in the pinned implementation.
The bridge still performs the native call; it does not suppress its BP work.

Production bridge and actual Switch Memory contracts pass ASan, fatal UBSan
and LeakSanitizer in both modes: 68,721 valid calls and 949 diagnosed SIGABRT
refusals each. This includes 3,180 Fog color/range calls and the unchanged
65,541 ZCompLoc calls. All single-bit deviations, unsupported types,
non-finite/extreme parameters, unreadable/truncated color spans and all
thirteen unadmitted hybrids from the union of observed components refuse.
CPU, memory, lookup ordering, native f64-to-f32 argument bits and colors are
checked. Four private mutants are rejected: removing the new tuple,
admitting component hybrids, narrowing the type guard, and forwarding f2
instead of f1.

The pinned native function body and register macro execute with only state
storage and BP transport seams replaced. All 3,072 fixtures cover three
tuples and every byte value of each RGBA channel, asserting every BP word
and state preservation, plus the existing ZCompLoc checks. The test script
now verifies both extracted sources against their original Git blobs.
The full desktop GPU/capture/replay suite passes. These tests establish
native arguments and BP semantics; they do not claim fresh Switch pixels.

Candidate `bb00520` passes the separate private rendered-build
and SDK gate: immutable offline image, 71 HLE files plus input/backend,
Discovery and capture checks, 85 unique scoped providers across 273 explicit
link inputs, 103 strong functions and three GX objects retained. Original
private patch bytes and nanosecond mtimes remain preserved.

All eight published checks in the complete paginated exact-HEAD rollup
complete successfully before [PR #363](https://github.com/yashin-sh/WiiCompiled-Switch/pull/363)
merges at 2026-10-10T13:17:53Z, main `2aab90f`.
The private NRO is 74,436,664 bytes, SHA-256
`225a1f44701ba7e9284409e714406f33b2aba7b6e0da95824c483a1c7281274b`. Full SD readback verifies
`sdmc:/switch/WiiCompiled-Switch-fog-zero-bb00520.nro` at
2026-10-10T13:18:08.366110+00:00; the capture-disabled marker is
independently reread. The previous owned IA4 NRO is copied to a private
backup and SHA/size verified before removal. Corrected console execution,
fresh pixels and playability remain pending. Game data, generated callers,
NROs and raw diagnostics remain excluded.
