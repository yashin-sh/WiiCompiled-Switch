# Bounded Fog and prepared ZCompLoc bridges (2026-10-03)

The latest [accepted console run](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
returned from AlphaCompare, then stopped at unsupported `GXSetFog`
`0x801722CC`. The user confirmed a black screen followed by an error.
This candidate implements that measured Fog call and prepares the next missing
`GXSetZCompLoc` `0x80172858` from the checked caller. Neither new bridge has
returned on the console yet. The preceding hardware result remains authoritative.

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

It checks the raw type and all f64 bits before narrowing, resolving memory or
calling native graphics. Different types or floating representations report
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

The checked local generated caller forecasts, after Fog:

1. Existing translated `GXSetFogRangeAdj` `0x80172658`, arguments `(0,0,0)`.
   Its disabled branch does not dereference the table, emits BP `E8000156`,
   and writes its existing guest state. No replacement bridge is added.
2. Prepared ZCompLoc `0x80172858`, r3 **1**.
3. Existing DstAlpha bridge `0x8017295C`, arguments `(0,0)`.
4. Return to the outer caller and its existing pixel-state setup: ColorUpdate,
   AlphaUpdate, Dither and DstAlpha.

This is a static forecast, not a console observation or an exhaustive list of
work before a first image. No new generated game code is published.

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
Exact-code remote workflows and the private Rendered Discovery NRO are the
next build gates. Console acceptance still requires an exact NRO transfer,
fresh attributable reports and later coherent execution beyond these calls.
Recognizable pixels and sustained playability remain unproven.
