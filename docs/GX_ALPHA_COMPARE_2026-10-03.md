# Bounded GXSetAlphaCompare bridge — 2026-10-03

## Observed trigger

The [accepted color/table run](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
crosses all twelve color/table calls and reaches DIRECT `0x80172088`,
GXSetAlphaCompare, `(7,0,0,7,0)`, dispatch 700091, LR `0x80240F9C`,
r1 `0x80398FC8`, stage BlendMode. Black output and a crash persist.
This separate candidate implements that observed function. Fog and later
stateful setters remain hard stops.

WiiCompiled/Aurora pin: `a135beb201042b20f390c6695ca6b26768820fb4`.
The pinned wrapper is `runtime/src/hle/gx/gx_pixel.cpp:66`; the native setter
is Aurora `lib/dolphin/gx/GXTev.cpp:128`, with legal enums from `GXEnum.h`.

## Contract and ownership

The void CpuContext bridge consumes r3/r6 as compare enums 0..7, r5 as alpha
operator 0..3, and r4/r7 as full unsigned reference words. It validates the
complete enum words before narrowing, native forwarding or host-state writes.
References follow the pin's u8 conversion for any u32, including 256 and
UINT32_MAX; they are not restricted to 0..255 at the CPU boundary.

Null CPUs have no effects. Non-null calls publish `RMCP01_GX_SET_ALPHA_COMPARE`
before guards. An unsupported enum diagnoses `GX_SET_ALPHA_COMPARE_UNPROVEN_ARGS`,
target `0x80172088` and the unchanged CPU, then aborts before native/flag work.
The bridge preserves the entire CPU and does not access guest memory.

Rendered mode sets the **existing** `g_alphaCompareValid` bool before calling
Aurora once. Its single production owner remains
`source/rendered_fast_track_graphics.cpp`; renderer initialization resets it.
Pinned `EnsureDefaultGxAlphaCompare` in `gx_stream_common.h` consumes the same
flag to avoid replacing an explicitly set comparison with ALWAYS/AND/ALWAYS
before a subsequent draw. A new disconnected flag or omission of this write
would lose that semantic effect. Headless mode retains guards/stages, without
native calls or a rendered-state dependency.

Aurora packs references at bits 0..7/8..15, comparisons at 16..18/19..21 and
operator at 22..23, writes BP register 0xF3, and sets bpSent=1. Aurora owns those
effects; the bridge adds no guest GXData mirror, dirty flag, frame helper,
presentation or reconstructed command cache.

## Validation and CI

[`test-gx-alpha-compare.sh`](../scripts/test-gx-alpha-compare.sh) executes the
actual bridge/direct trait with the real pinned Aurora signature in rendered
modes 0/1, ASan/UBSan fatal and LSan active. Each mode passes **331,264 valid
calls and 38 diagnosed SIGABRT refusals**. Coverage includes:

- all 256 legal compare/operator combinations, every byte in each reference
  position, asymmetric counterpart values and wide raw references;
- every 65,536 reference pair on the console's ALWAYS/AND/ALWAYS combination;
- initial validity flag false and true, required publication before native
  forwarding, exactly one matching native call in rendered mode;
- invalid raw enum words including 256, high-bit values and UINT32_MAX,
  independently and together, with exact diagnostic and stage;
- full CPU byte preservation, including refusal writes after reporting via
  shared CPU storage, and null calls preserving stage/counters/flag.

GNU linker abort instrumentation checks flag preservation again immediately
before delegating to the real libc abort. A native attempt in a refusal child
has a distinct exit status, preventing assertion SIGABRT from becoming false
acceptance. Four temporary mutation checks reject late flag publication,
narrowed enum guards, flag writes after reporting, and reversed reference
conversion. These are forwarding/context contracts, not native BP decoding,
rendering or hardware acceptance.

`build-switch` executes this eleventh host contract. `fast-track-startup`
retains the link-only synthetic probe and checks its and the bridge's strong
text symbols. Makefile/CMake globs include the source; the rendered AArch64
syntax gate discovers it automatically.

To anticipate the next diagnostic round, the unsupported Fog report now
captures actual f1..f4 **f64 bits** and a checked complete four-byte color
range on arrival at `0x801722CC`. Unreadable color is explicitly marked NO;
zero byte placeholders then do not establish guest data. No float narrowing,
guessed constants, Fog implementation or continuation past its hard stop is
introduced. The checked caller forecast Fog after AlphaCompare and existing
ZMode; the later console run now reaches that boundary and captures its data,
while Fog itself remains unsupported.

- [x] AlphaCompare host contract and four temporary mutation checks.
- [x] New shell script syntax/ShellCheck and workflow actionlint.
- [x] Complete candidate rendered AArch64 syntax gate, including Fog diagnostics.
- [x] Exact candidate public GitHub workflows and synthetic symbol checks.
- [x] Private Rendered Discovery build and native provider/flag ownership check.
- [x] Exact NRO transfer and fresh attributable progression beyond AlphaCompare.

The [later console result](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
now establishes AlphaCompare returned on (7,0,0,7,0), followed by existing ZMode
and the missing Fog boundary. Fog type 0, exact f64 parameter bits and readable
RGBA 255,255,255,255 are captured. Other inputs and recognizable game pixels
remain unproven; visual observation for this run is still pending.

## Completed build evidence

Integrated code `1a8c092fbc7f1b6c4f9f0562a5b5d9a295f406eb` passed all five
GitHub workflows and six jobs, including all eleven host contracts. Actual CI
logs confirm the AlphaCompare valid/refusal counts in both rendered modes.
The private Rendered Discovery build completed with exit 0 at
2026-10-03 14:42:13 UTC (16:42:13 Europe/Paris), after 29 minutes 52 seconds.
It uses Rendered/Discovery ON, three jobs, network disabled and immutable image
`sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`.

The NRO is **73,355,320 bytes**, SHA-256
`7032c756f4f0872334aea0a4421a8633e8d76d9ed1c004cf2d59fafa87b5b310`.
All 27 required strong text symbols are present. Source hashes stayed unchanged
during compilation; dependency pins, existing WiiCompiled patch bytes/mtimes
and the preceding coordinate, audit, scalar and color/table NROs are preserved.
Artifact, complete logs and detailed metadata remain local under
`.deps/network-tests/gx-alpha-compare/`.

A fresh scan of 223 explicit host link inputs, nineteen Rust archives and seven
named libraries in the same image verifies the new bridge's native and flag
references. `GXSetAlphaCompare` has exactly one strong provider, Aurora
`libm3_aurora_gx.a:GXTev.o`. `g_alphaCompareValid` has exactly one owner, the
existing `rendered_fast_track_graphics.o`. The bridge defines neither a native
replacement nor another flag. This proves ownership of these two symbols, not
general duplicate-definition freedom under the private link's broad policy,
native BP decoding or visible pixels. The later console run accepts the
observed AlphaCompare return, with the scope and limits documented above.
