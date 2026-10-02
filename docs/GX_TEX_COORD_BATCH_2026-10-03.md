# Bounded texture-coordinate batch — 2026-10-03

## Observed frontier and scope

The matrix candidate `02010aaaa974581f49ed2dddfd02ebf0e2bc0e96`, NRO SHA-256
`e666413d2c76a2ef9a5a278a4feb4a1e94ae01632464b643a62f885905bb656e`,
returned through ten type-0 matrix loads and Gen2 coord 0. The next direct
frontier was `GXSetTexCoordScaleManually (0x80171180)` at dispatch 605350,
99,513 ms after the first dispatch, `(coord,enable,S,T)=(0,0,0,0)`,
LR `0x80241334`, stage `RMCP01_GX_SET_TEX_COORD_GEN2`. Raw reports remain
local under `.deps/network-tests/gx-load-tex-mtx-imm/runs/20261002T174349Z`.

The pinned WiiCompiled/Aurora contract and local caller are audited in
`GX_TEX_COORD_NEIGHBORS_2026-10-02.md`. At pin
`a135beb201042b20f390c6695ca6b26768820fb4`, `func_802412C8` forecasts an
eight-coordinate loop with Gen2(coord,1,4,60,0,125), disabled Scale and Bias.
This candidate implements that bounded family together:

| Call | Accepted arguments |
| --- | --- |
| Scale, `0x80171180` | coord 0..7; enable 0/1; raw u32 sizes narrowed to native u16 |
| Bias, `0x801711FC` | coord 0..7; S/T enable 0/1 |
| Gen2, `0x8016E37C` | tuple (coord,1,4,60,0,125), only coord 0..7 varies |

Scale coord 0 disabled and Gen2 coord 0 are hardware observed. Other coords,
Bias and the enabled branches are audited and host-testable, but this batch
has no new hardware acceptance yet. Out-of-scope args record a durable
UNPROVEN_ARGS blocker and abort before native conversion or guest mutation.
Null CPU calls have no effects. Guards also exclude noncanonical booleans:
TARGET_PC makes GXBool a bool, whereas the guest mirror uses raw low bits.

## Native and guest behavior

Scale/Bias call the actual Aurora setter first. The headless branch omits
Aurora and retains the guest bookkeeping. Shared local mirror code then reads
the real guest GXData pointer from `0x803886C8`; it does not fabricate an address.

Scale updates `GXData+0x5E4`, changing only the coordinate's manual-enable bit.
When enabled it updates low 16 bits of S at `+0x108+coord*4` and T at
`+0x128+coord*4` with `(size-1)&0xFFFF`, then clears the halfword at `+2`.
Zero sizes retain the pin's unsigned underflow to `0xFFFF`; there is no clamp.
Disabled Scale changes only its manual-enable bit.

Bias changes only bit 16 of the same S and T words. It clears `+2` only if
that coordinate's manual-enable bit is set. There is no guest dirty-state
update, new frame activation, presentation or frame-work marker here. Gen2
has no guest GXData mirror; its existing native forwarding changes only the
permitted destination coordinate.

Mirror operations preserve the pin's order, completed writes and best-effort
return on missing/short/null backing. The code validates each individual
address/range immediately before that operation, not the whole structure
before any write. A later failure leaves earlier writes intact and skips
only the remaining mirror; it does not undo the native GX call.

One deliberate hardening is documented: additions use uint64_t and reject
wrap into low guest addresses, unlike the pin's unchecked u32 address maths.
Valid nonwrapping ranges keep the pinned masks/order. This does not turn
unimplemented calls into success or fabricate any graphics state.

## Contracts and integration

The executable host contract uses the actual Switch Memory implementation
with allocation-only synthetic mappings, independently encoded big-endian
bytes and byte-for-byte canaries. It checks all eight legal coordinates,
boolean combinations, size narrowing/underflow, native-before-mirror order,
neighboring bits and dirty-state preservation, CPU preservation, every completed
prefix on truncated/discontiguous backing, unaligned and exact end ranges,
2^32 overflow without aliasing into low canaries, and diagnosed argument aborts.
Gen2's other five argument guards remain independently tested.

The synthetic probe retains the three real bridges/direct traits without
executing fabricated game state. The public host-contract workflow includes
this batch; fast-track linking checks its actual retained bridge symbols.
No Nintendo-derived memory bytes are committed or used as test fixtures.

## Validation status

Code candidate `91a4a01b8e316f9010e9d31754e279065772f9f3` passed:

- all six GX executable host contracts in headless and rendered modes with
  ASan/UBSan; this coordinate contract executes 199 valid calls and 29
  diagnosed argument refusals per mode, plus null-CPU checks;
- all 10 local lint checks and all 28 local steps of the five required
  workflows; these are local equivalents, not a GitHub Actions run;
- the rendered AArch64 syntax gate for the GX bridges and Discovery
  diagnostics;
- the private Rendered Discovery build, including strong definitions of all
  three bridges and the actual Aurora Scale, Bias and Gen2 functions.

The build completed at `2026-10-02T22:56:35Z` (00:56:35 on October 3,
Europe/Paris). It produced a 73,297,976-byte NRO with SHA-256
`64ba837720f4e37cbd127c37a0e9bde6dc146ed229a92c8697b9c531a8984d08`.
Dependency pins, command, source hashes and validation results remain local
under `.deps/network-tests/gx-tex-coord-batch/`. The original nine-file
WiiCompiled integration patch was preserved byte-for-byte, SHA-256
`92984dd129257e15e006fb7df1d891b6a7799f88620ad560c7a9733d4c19d0f7`.

Hardware validation is pending. No new hardware crossing, presentation
effect or visible game image is claimed by the build. Acceptance must use
fresh reports tied to this exact NRO, a later distinct dispatch and
caller/control-flow evidence for the coordinate loop.

The generated caller forecasts entry to `func_80241380` after all eight
Gen2/disabled-Scale/disabled-Bias triples return. Without intervening
callbacks, that entry is 23 dispatch increments after the first Scale hit,
with LR `0x80240F98`, r3=7 and r8=125. The forecast next missing DIRECT call
is GXSetTevDirect (`0x80171B58`), stage 0, one dispatch later after the
existing GXSetNumTevStages bridge. These are static forecasts until new
hardware reports establish the actual frontier. Callback polling can add
dispatches or replace the global diagnostic stage; the control flow and
captured state must be assessed together with the counter.
