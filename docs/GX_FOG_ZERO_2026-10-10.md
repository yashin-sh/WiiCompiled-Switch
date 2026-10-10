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

Private rendered-build, exact-HEAD published CI, corrected NRO deployment
and console execution are pending for this candidate. The previous nine
private integration patches and generated game products remain excluded.
