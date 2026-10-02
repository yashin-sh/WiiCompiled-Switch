# Indirect texture matrix and adjacent scalar candidate (2026-10-02)

The user authorized the next candidate after the exact hardware frontier in
`HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_SET_IND_TEX_MTX_FRONTIER.md`.
The scope is the observed GXSetIndTexMtx boundary plus the adjacent audited
GXSetIndTexCoordScale setter. No other guest-memory boundary is pre-ported.

## Pinned contract

WiiCompiled remains `a135beb201042b20f390c6695ca6b26768820fb4`.
Its `runtime/src/hle/gx/gx_indirect.cpp` maps:

| Target | Inputs | Forwarding |
| --- | --- | --- |
| `0x80171814` GXSetIndTexMtx | r3 selector, r4 guest matrix address, r5 exponent | Read six big-endian float32 values; cast selector to GXIndTexMtxID and exponent to s8 |
| `0x80171968` GXSetIndTexCoordScale | r3 stage, r4 S scale, r5 T scale | Forward all three enum casts unchanged |

Hardware observed only the first matrix call: r3=1, r4=`0x802581F8`, r5=1.
The coefficients were not captured. Tests use independently generated data;
they do not claim to reproduce that matrix.

Pinned Aurora's `lib/dolphin/gx/GXBump.cpp` maps regular/S/T selectors onto
three matrix slots, converts `1024.f * coefficient` to signed 32-bit fields,
packs three BP registers with the adjusted exponent, writes them and sets
`bpSent=1`. The bridge calls this implementation with a decoded host array;
it neither guesses coefficients nor duplicates Aurora's register packing.

The coefficient guard preserves all defined float-to-s32 conversions. It
stops before a nonfinite or out-of-range scaled value reaches Aurora. The
lower bound is inclusive -2^31 and the upper bound exclusive 2^31. Invalid
memory, including a range crossing the end of the guest address space, also
retains a durable hard stop. Inputs are never clamped or substituted.

The coordinate-scale implementation selects one of two cached words according
to the stage, updates the S/T four-bit fields, emits the BP word and sets
`bpSent=1`. Valid stages are 0..3 and valid scale enums 0..8. The bridge keeps
the pinned casts; stage and field behavior remains in actual Aurora.

Both wrappers preserve guest CPU registers and have separate stages:
`RMCP01_GX_SET_IND_TEX_MTX` and `RMCP01_GX_SET_IND_TEX_COORD_SCALE`.
Invalid memory and coefficients have distinct diagnostic stages. Non-rendered
builds decode/validate the matrix and publish stages without issuing GX calls.

## Validation

`scripts/test-gx-indirect-batch.sh` compiles the real bridges, native traits,
CpuContext, Aurora types and Switch `memory_switch_slice.cpp`. A small host
allocation seam replaces Horizon allocation; GX sinks observe forwarding.
The checked Memory implementation and its actual big-endian reads run intact.

Rendered and non-rendered contracts execute under ASan/UBSan, including
float-cast-overflow instrumentation. They check:

- exact six-value decoding and row order, signed zero and distinct coefficients;
- regular/S/T matrix selectors and signed eight-bit exponent narrowing;
- unaligned mapped access, a matrix ending exactly at the region boundary,
  and preservation of all CPU bytes and guest matrix bytes;
- one native call per rendered invocation and stage publication before it;
- the full cross product of four legal stages and nine S/T scale enums;
- null handling and diagnostic-only non-rendered behavior;
- child-process durable reports followed by SIGABRT for uninitialized, null,
  unmapped, truncated and wrapping matrix ranges;
- NaN, infinities and out-of-range coefficients in each of the six positions,
  plus the valid conversion limits.

The synthetic fast-track probe retains both direct-dispatch instantiations and
both bridge symbols. The build workflow additionally executes the scalar and
indirect host contracts on a separate host runner with the pinned Switch
header patch. These checks prove decoding/forwarding, not Aurora FIFO/backend
effects. The rendered syntax gate, private rendered build and exact-NRO
hardware run remain required.

## Runtime acceptance

The baseline is code candidate `1406039e9addbeeb7889002faf96cb870934fba3`, NRO
SHA-256 `cfa889d80b8e3a6b4435131142926fcc4801c2dd8b955940b5c73ca408108027`.
It crosses GXSetClipMode and stops at the first indirect matrix call.

The local translated caller forecasts three matrix calls followed by four
coordinate-scale calls. Each boundary needs a later distinct dispatch or
durable milestone to establish hardware progression. GXSetIndTexCoordScale,
GXSetDither and GXSetDstAlpha remain unvalidated on hardware until reached.
Unknown boundaries continue to hard-stop.

Validation results and the final NRO identity will be recorded after the gates.
