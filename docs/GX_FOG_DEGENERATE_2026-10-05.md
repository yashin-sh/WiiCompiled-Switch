# Bounded Mii texture Fog tuple — 2026-10-05

The [PADReset console run](HARDWARE_RESULTS_2026-10-05_PAD_RESET_MII_FOG_FRONTIER.md)
passes reset mask `0x70000000`, then stops at `GXSetFog` `0x801722CC`,
type 0, parameters `(1,1,0,0)`, transparent black, dispatch 635434.
The bridge previously accepted only the initial `(0,1,0.1,1)` call.
This correction admits the exact second tuple observed during Mii texture
preparation. The console still shows a black screen followed by an error.

## Pinned contract and scope

Pinned WiiCompiled forwards type r3, guest RGBA pointer r4 and f1..f4 `.d`
narrowed to float. Aurora's perspective branch explicitly uses `A=0, B=0.5,
C=0` when either depth range or fog range is zero. For `(1,1,0,0)`, both ranges
are zero: normalization terminates without division, the scaled mantissa is
finite and fits u32, and the exponent becomes 1. Native output is exactly
`EE000000`, `EF40000F`, `F0000001`, `F1000000`, then `F2rrggbb`, with `bpSent=1`.
The alpha byte is forwarded but is absent from FOGCLR, as in the pinned code.

The bridge accepts type 0 and either complete observed f64 tuple:

| Call | f1 / start bits | f2 / end bits | f3 / near bits | f4 / far bits |
| --- | --- | --- | --- | --- |
| Initial | `0000000000000000` | `3FF0000000000000` | `3FB99999A0000000` | `3FF0000000000000` |
| Mii texture | `3FF0000000000000` | `3FF0000000000000` | `0000000000000000` | `0000000000000000` |

It compares complete tuples before float narrowing, guest lookup or native
work. Hybrid combinations, signed zero and other finite/non-finite representations
retain `GX_SET_FOG_UNPROVEN_ARGS` and diagnosed aborts. Color range checks,
byte order, CPU/memory preservation, native argument forwarding and diagnostic
stage are unchanged. Broader Fog parameters remain outside this bounded correction.

## Validation

The expanded bridge contracts exercise both tuples in both rendered modes:
**67,661 valid calls and 630 diagnosed SIGABRT refusals per mode**.
These include 2,120 color/range Fog calls, the existing 65,541 ZCompLoc calls,
all single-bit departures from both f64 tuples, six whole-tuple hybrids,
unsupported types, non-finite/extreme arguments and invalid color ranges.
ASan, fatal UBSan and LeakSanitizer remain active. Complete CPU and guest-memory
preservation, tuple-before-lookup ordering and null contexts are checked.

The pinned native Fog function body and register macro execute verbatim with
only state storage/transport replaced. **2,048 native fixtures** verify all
five BP words across both tuples and every RGBA byte domain, plus the existing
ZCompLoc preservation checks. Four independently compiled mutants are rejected:
rejecting the new tuple, admitting component hybrids, narrowing the type guard
and forwarding the wrong FPR.

All eighteen local host suites, both AArch64 modes, the full synthetic NRO,
script/workflow lint and formatting
pass. The existing Fog/ZComp synthetic probe and both production symbols are
retained. Original upstream patch bytes/mtimes and WiiCompiled/Dawn/Mesa pins
are preserved. Remote acceptance is tracked in [PR #315](https://github.com/yashin-sh/WiiCompiled-Switch/pull/315).

The private Rendered Discovery build from code `d2c8abe` exits 0 at
**2026-10-05 20:18:52 UTC**. Its NRO is **73,494,584 bytes**, SHA-256
`7a0463aaf070422c1d4ae1191b22a0304616c1a88194eadb4ed389fcb5235a4f`.
All fifty required strong symbols are retained. Twenty-three scoped symbols,
including native `GXSetFog` and its bridge, have unique expected providers
across 231 host inputs, nineteen Rust archives and seven named libraries.
The checked native Fog source matches the pinned original byte-for-byte.
Broader symbols and implicit compiler libraries are outside this scoped audit.
Console deployment and remote checks are tracked in PR #315. The new tuple's
console return remains unaccepted until fresh progression evidence is retrieved.
