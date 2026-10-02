# Ambient channel color candidate (2026-10-02)

The user authorized this candidate after the GXSetChanAmbColor frontier in
`HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_SET_CHAN_AMB_COLOR_FRONTIER.md`.
The scope is only the observed direct boundary `0x8017039C`, captured with
r3=4 and r4=`0x80398FD0`. The pointed-to color was not captured; tests use
independently generated RGBA bytes.

## Pinned contract

WiiCompiled remains `a135beb201042b20f390c6695ca6b26768820fb4`.
Its `runtime/src/hle/gx/gx_lighting.cpp` activates the Aurora frame, reads
one big-endian word from the guest color pointer, decodes RGBA from high to
low bytes, then calls GXSetChanAmbColor with the unchanged channel cast.
The bridge preserves this order and all guest CPU bytes.

Pinned Aurora's `lib/dolphin/gx/GXLighting.cpp` expands channel 4
(GX_COLOR0A0) into COLOR0 and ALPHA0, and channel 5 similarly into COLOR1 and
ALPHA1. It retains channel validation, cached ambient colors, XF writes to
registers 0xA/0xB and `bpSent=0`. The bridge calls this implementation; it
does not duplicate those effects or clamp the channel.

Rendered mode publishes `RMCP01_GX_SET_CHAN_AMB_COLOR` and calls the existing
EnsureAuroraFrameActive helper before accessing guest memory. Uninitialized,
null, unmapped, truncated or wrapping color ranges publish the invalid-color
stage and durable report, then abort before issuing GX calls. Headless mode
publishes the stage without accessing the pointer or issuing frame/GX calls,
matching the existing material-color bridge. Null CPU pointers have no effect.

## Validation

`scripts/test-gx-chan-amb-color.sh` compiles the actual bridge, native trait,
CpuContext, Aurora types and Switch `memory_switch_slice.cpp`. A host allocation
seam replaces Horizon allocation; frame and GX sinks observe call ordering
and forwarding. Rendered and headless contracts pass under ASan/UBSan.

Tests cover all six legal channels, independently written big-endian colors,
unaligned pointers, the last valid four bytes of a region and of the guest
address space, active/inactive frames, exact call counts, stage ordering,
CPU/guest-color preservation and null CPU handling. Deliberate color replacement
inside the test frame helper distinguishes ensure-before-read from the reverse
order. Invalid-pointer child processes must produce the exact durable report
and SIGABRT after frame activation and before GX forwarding.

The synthetic fast-track probe retains the direct native trait and bridge.
The build workflow executes scalar, indirect and ambient host contracts.
Host sinks prove decoding and forwarding, not actual Aurora frame creation,
FIFO/backend effects or visual correctness. Rendered syntax, the private
rendered build and an attributable exact-NRO hardware run remain required.

## Runtime acceptance

The baseline NRO has SHA-256
`9baf0d8141be506e2c75bbc63217466c6412f777cf6b3aea95419d9deddedc4b`.
It crosses the indirect texture matrix and coordinate-scale calls, then stops
at GXSetChanAmbColor. This candidate needs a later distinct dispatch or durable
milestone after its ambient-color stage to establish hardware progression.
Other unknown boundaries continue to hard-stop. GXSetDither and GXSetDstAlpha
remain unreached. Hardware acceptance is pending.
