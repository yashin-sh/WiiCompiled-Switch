# Bounded Fog and prepared ZCompLoc bridges (2026-10-03)

The preceding [accepted console run](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
returned from AlphaCompare, then stopped at unsupported `GXSetFog`
`0x801722CC`. The user confirmed a black screen followed by an error.
This candidate implements that measured Fog call and prepares the next missing
`GXSetZCompLoc` `0x80172858` from the checked caller. The [later console run](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
now establishes both new bridges returned on the executed inputs and the
following pixel setup; depth-texture LOD is the new boundary. Earlier hardware
records retain their original scope.

## Fog scope and pinned semantics

The pinned WiiCompiled wrapper consumes type from r3, color pointer from r4
and float arguments by converting f1..f4 `.d` to f32. It decodes the guest
four-byte RGBA and calls Aurora `GXSetFog`; it has no explicit guest-state
mirror or validity flag to update.

The first Switch bridge admits type **0 / GX_FOG_NONE** and the exact measured
f64 representations:

| Parameter | Accepted f64 bits | Native f32 bits |
| --- | --- | --- |
| startZ / f1 | `0000000000000000` | `00000000` |
| endZ / f2 | `3FF0000000000000` | `3F800000` |
| nearZ / f3 | `3FB99999A0000000` | `3DCCCCCD` |
| farZ / f4 | `3FF0000000000000` | `3F800000` |

It checks the raw type and all f64 bits before narrowing, the bridge's color
lookup or native graphics. The durable diagnostic may independently take a
checked color snapshot when reporting a refusal. Different types or floating representations report
`GX_SET_FOG_UNPROVEN_ARGS` and abort without continuing. This includes signed
zero, non-finite values and f64 deviations that would disappear on narrowing.
These refusals bound the initial implementation; they do not claim that every
refused tuple is invalid in the Wii SDK.

`Memory::GetPointer(r4, 4)` validates the complete guest range. Unreadable colors
report `GX_SET_FOG_UNREADABLE_COLOR` and abort. Readable colors are forwarded
from individual RGBA bytes, including unaligned addresses and mapped address
zero. The actual pointer and floats are consumed; native arguments are not
replaced by constants. The stage is `RMCP01_GX_SET_FOG`.

Aurora still computes coefficients for NONE. An unconditional no-op would lose
five BP register writes and `bpSent=1`. For the accepted finite tuple the
normalization terminates, B mantissa conversion is in u32 range, and the
native fixture emits `EE03CE38`, `EF471C82`, `F0000002`, `F1000000`, then
`F2rrggbb`. Alpha is passed but is not encoded in FOGCLR. More general Fog
parameters require a separate arithmetic and runtime audit.

## ZCompLoc and the checked continuation

Pinned `GXSetZCompLoc` forwards r3 as `GXBool`. In the Switch TARGET_PC build
this is **bool**, so the entire u32 word is tested for nonzero; converting
through u8 would incorrectly turn 256 or `0x80000000` into false. The new
bridge preserves that conversion and publishes `RMCP01_GX_SET_Z_COMP_LOC`.
It accepts all u32 inputs, changes no CPU/guest bytes and performs no lookup.
Native Aurora updates only PE control bit 6, writes the PE register and sets
`bpSent=1`.

The checked local generated caller orders, after Fog:

1. Existing translated `GXSetFogRangeAdj` `0x80172658`, arguments `(0,0,0)`.
   Its disabled branch does not dereference the table, emits BP `E8000156`,
   and writes its existing guest state. No replacement bridge is added.
2. ZCompLoc `0x80172858`, r3 **1**.
3. Existing DstAlpha bridge `0x8017295C`, arguments `(0,0)`.
4. Return to the outer caller and its existing pixel-state setup: ColorUpdate,
   AlphaUpdate, Dither and DstAlpha.

Fresh console progression now accepts this executed continuation using first hits,
checked caller flow and later coherent state. This is not a standalone return
trace or an exhaustive list of work before a first image. No new generated
game code is published.

## Contracts and build acceptance

`bash scripts/test-gx-fog-z-comp.sh` executes real bridges and the Switch
memory-range implementation in rendered modes 0 and 1, with ASan, fatal UBSan
and LeakSanitizer active:

- **66,601 valid calls per mode:** 1,060 Fog color/range cases and 65,541
  ZCompLoc bool-conversion cases, including every low u16 and wide words.
- **312 diagnosed SIGABRT refusals per mode:** all 256 individual f64 bit
  deviations, non-finite/extreme/signed values, unsupported full type words,
  unmapped/short/wrapping ranges, uninitialized and reset memory.
- Null contexts have no effects. CPU bytes and guest backing are preserved.
  Guard pages catch overreads; shared memory catches writes after reporting.
  Wrapped lookups check tuple-before-memory ordering and length 4. Native
  sinks cannot satisfy an expected refusal using an unrelated assertion.
- The pinned native Fog and ZCompLoc function bodies and register macro are
  copied verbatim into a temporary test translation unit. Only command
  transport/state storage are replaced. **1,024 native Fog fixtures** check
  exact five-register output across each RGBA byte domain; ZComp checks
  preservation of all other PE bits.

The new host contracts, rendered AArch64 syntax, new-script ShellCheck,
actionlint and new/changed C++ formatting pass locally. CI executes this
**twelfth** host script and retains both bridge symbols through a synthetic
link probe. Synthetic startup does not execute fabricated game arguments.
All five workflows and six jobs passed on exact bridge code `c329b6d0`.
Actual CI logs confirm both bridge modes and the pinned native fixture. The
private Rendered Discovery build passed at **2026-10-03 18:55:08 UTC**, with
**31 required strong text symbols**. Its NRO is **73,396,280 bytes**, SHA-256
`652afed471c2fc8ba9aa6735612fbe1ac69295aecf7b9a0dbfd6505dfb1024fb`.
A fresh scan of **225 explicit host inputs, nineteen Rust archives and seven
named image libraries** found exactly one Aurora `GXPixel.o` provider for
each of `GXSetFog` and `GXSetZCompLoc`. New bridge objects define their
Switch entry and reference the native symbol; they do not replace it. This is
a scoped ownership check, not a general duplicate-definition guarantee.
Dependency pins, the nine-file user integration patch bytes/mtimes and all
preceding hardware NROs are preserved.

Four temporary incorrect variants were rejected: narrowed type guard, wrong
FPR index, reversed RGBA and u8 narrowing before ZCompLoc bool conversion.

The initial GitHub host-contract job took **19 min 03 sec** against a 20-minute
limit. The subsequent host-test-only change disables core dumps with
`PR_SET_DUMPABLE=0` in each expected-abort child: pipe-based core collectors
can ignore `RLIMIT_CORE=0`. The test still requires a diagnosed real SIGABRT,
checks shared CPU/memory afterwards and leaves parent LeakSanitizer active.
The local rendered probe completed all 66,601 valid calls and 312 refusals in
about two seconds of execution. All five workflows and six jobs also pass on the optimized-test revision
`e24480511513899e7ae242343f16cccfebfa244e`, with the same twelve contracts.
Actual CI logs confirm 66,601 valid calls / 312 refusals in both modes and
the 1,024 native fixture cases. The host-contract job took **10 min 39 sec**
in that run, compared with 19 min 03 sec in the initial run. These are two
observed CI durations, not a guaranteed runner performance bound.
The fresh private build at **19:00:14 UTC** performed no compilation and
produced the exact same NRO digest as `c329b6d0`; the Switch sources and
binary are unchanged by the host-test optimization.

## Console acceptance

The exact NRO transferred with exit 0 at **19:20:07 UTC**. Fresh verified reports
establish the observed Fog call and ZCompLoc(1) returned, through existing
disabled FogRangeAdj and pixel setup. See the [hardware result](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
for attribution, context and limits. The next durable stop is
`GX_INIT_TEX_OBJ_LOD_INVALID_DESCRIPTOR` at `0x80170A4C`, a 4×4 depth texture
whose valid format 22 is missing from the structural validator. Its native
init passed; LOD has not returned. Current visual observation is pending.
Recognizable pixels and sustained playability remain unproven.
