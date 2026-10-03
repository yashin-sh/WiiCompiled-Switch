# Bounded TEV scalar batch — 2026-10-03

## Observed trigger and scope

The accepted [audit hardware run](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md),
code `b3484117`, NRO `7ecbc8a9...`, reaches DIRECT GXSetTevDirect
(`0x80171B58`), TEV stage ID 0, dispatch 608381, elapsed_ms 107925,
LR `0x80240F98`, stage `RMCP01_GX_SET_NUM_TEV_STAGES`. Arrival is observed;
Direct has not returned. The user confirmed a black screen.

This bounded candidate covers six per-stage setters. Existing
GXSetNumTevStages and GXSetTevOrder remain on their existing paths.
GXSetTevKColor, GXSetTevColor and GXSetTevSwapModeTable are outside the lot;
the first two introduce guest-pointer reads.

The wrapper/Aurora pin is
`a135beb201042b20f390c6695ca6b26768820fb4`. Attribution uses
`runtime/src/hle/gx/gx_tev.cpp`, `gx_indirect.cpp`, bundled Aurora
`GXTev.cpp`, `GXBump.cpp`, `GXEnum.h`, `GXManage.cpp` and the local generated
caller `func_80241380`. The broader pinned analysis is in
[GX_TEV_NEIGHBORS_2026-10-03.md](GX_TEV_NEIGHBORS_2026-10-03.md).

## Argument contract

Every bridge is void, takes a CpuContext pointer, reads original unsigned
32-bit arguments from r3 onward and preserves the guest CPU. A null CPU has
no effects. Bridge symbols are
`mkw_switch_hle_gx_set_tev_{direct,color_in,color_op,alpha_in,alpha_op,swap_mode}(CpuContext*) noexcept`.

The six implementations are in
[`gx_set_tev_direct_hle_bridge.cpp`](../source/gx_set_tev_direct_hle_bridge.cpp),
[`gx_set_tev_color_in_hle_bridge.cpp`](../source/gx_set_tev_color_in_hle_bridge.cpp),
[`gx_set_tev_color_op_hle_bridge.cpp`](../source/gx_set_tev_color_op_hle_bridge.cpp),
[`gx_set_tev_alpha_in_hle_bridge.cpp`](../source/gx_set_tev_alpha_in_hle_bridge.cpp),
[`gx_set_tev_alpha_op_hle_bridge.cpp`](../source/gx_set_tev_alpha_op_hle_bridge.cpp) and
[`gx_set_tev_swap_mode_hle_bridge.cpp`](../source/gx_set_tev_swap_mode_hle_bridge.cpp).
Their presence is implementation evidence, not completed validation.

| Setter / PAL address | Register arguments | Bounded legal domain |
| --- | --- | --- |
| Direct `0x80171B58` | r3 stage | stage 0..15 |
| ColorIn `0x80171CE0` | r3 stage, r4..r7 a,b,c,d | stage 0..15; every color input 0..15 |
| ColorOp `0x80171D60` | r3 stage, r4 op, r5 bias, r6 scale, r7 clamp, r8 output | stage 0..15; op 0,1,8..15; bias 0..2; scale 0..3; output 0..3; any u32 clamp |
| AlphaIn `0x80171D20` | r3 stage, r4..r7 a,b,c,d | stage 0..15; every alpha input 0..7 |
| AlphaOp `0x80171DB8` | r3 stage, r4 op, r5 bias, r6 scale, r7 clamp, r8 output | stage 0..15; op 0,1,14,15; bias 0..2; scale 0..3; output 0..3; any u32 clamp |
| SwapMode `0x80171FD0` | r3 stage, r4 raster selector, r5 texture selector | stage 0..15; both selectors 0..3 |

These are legal SDK domains, not only the static caller's zero/default
tuples. The operation guard distinguishes legal Color comparisons from the
two Alpha A8 comparisons. Bias and scale domains apply even when native
comparison encoding ignores their values. Stage IDs are not restricted by
the current active TEV stage count: the caller sets that count to 1 and
still initializes stages 0..15.

TARGET_PC makes GXBool a C++ bool. Clamp conversion therefore accepts the
entire u32 domain: zero is false, every nonzero value is true, including
256 and UINT32_MAX. No byte truncation or low-bit interpretation is valid.
The other legal enum arguments are forwarded without byte narrowing or
replacement by fixed ADD/default selectors.

These guards deliberately bound the candidate more strictly than several
pinned wrappers. The pin guards stage IDs on the five `gx_tev.cpp` entries,
output registers on ColorOp/AlphaOp and swap selectors on SwapMode. It does
not guard the other enums, and its Direct wrapper has no stage guard.
Each non-null call publishes its `RMCP01_GX_SET_TEV_*` diagnostic stage
before the guard. The candidate diagnoses any argument outside its bounded
domain with its specific `GX_SET_TEV_*_UNPROVEN_ARGS` report, the constant
PAL target address and unchanged raw CPU, then aborts before conversion or
native effects. A null CPU does not publish a stage. It does not claim to
reproduce the pin's malformed-input
early returns or unguarded enum behavior outside the documented scope.

## Native state and CPU effects

Rendered bridges forward once to the corresponding actual Aurora function.
Headless bridges validate/preserve the same guest contract without native GX
work. No bridge-level guest-memory lookup, GXData mirror, guest dirty-bit update,
frame activation, presentation or frame-work marker is added.

Aurora ColorIn/ColorOp share `__gx->tevc[stage]` and
AlphaIn/AlphaOp/SwapMode share `__gx->teva[stage]`. Their cached command
words preserve address bytes and fields written by the neighboring setter.
Direct forwards to Aurora GXSetTevIndirect with direct-mode defaults, emitting
a zero indirect payload at BP address `stage+0x10`. It does not call the
separate guest indirect wrapper or inherit that wrapper's clamping.

Each native operation emits its BP register command through Aurora's FIFO
and sets host `__gx->bpSent`. Color/AlphaOp preserve the native ADD/SUB branch
and comparison encoding, including forced comparison bias and converted clamp.
Native FIFO writes can use display-list recording and bypass the instrumented
guest GX_HLE_FIFO_Write counter. An unchanged guest FIFO count is not proof
that these native commands did not execute. A new independent bridge cache or
reconstructed register word would lose this shared-state contract.

## Static caller and future hardware proof

After GXSetNumTevStages(1), `func_80241380` loops over all s=0..15:

| Order | Forecast call / arguments |
| --- | --- |
| 1 | Direct(s) |
| 2 | Existing Order(s,255,255,255) |
| 3 | ColorIn(s,15,15,15,15) |
| 4 | ColorOp(s,0,0,0,1,0) |
| 5 | AlphaIn(s,7,7,7,7) |
| 6 | AlphaOp(s,0,0,0,1,0) |
| 7 | SwapMode(s,0,0) |

This is 96 new-setter calls plus 16 existing Order calls, not hardware
acceptance of that loop. After it, the forecast first missing boundary is
GXSetTevKColor `0x80171ED4`, with ID 0 and pointer r1+20, LR
`0x80240F98`, on the same guest fiber and nested stack. With the previously
observed r1 `0x80398FB8`, the pointer expression would be `0x80398FCC`;
it is a forecast, not a captured future pointer. Its four RGBA bytes come
from guest small-data memory and are neither assumed nor fabricated here.

Without callback dispatches, a first mapped Direct entry at count D would
be followed by the KColor unknown-DIRECT record at D+111. Known calls count
before invocation; an unknown DIRECT dispatch does not add another increment. Polling
can add callbacks and change diagnostic stages. Assess actual arguments,
LR/stack/fiber, executed control flow and later durable state together,
rather than treating an exact counter delta as unconditional.

A later fresh KColor boundary matching this path could establish return of
all 16 iterations and the six new setters on these forecast tuples. First-hit
tracing would still not contain 96 individual return records. It would not
hardware-validate alternate inputs, comparison operations, noncanonical clamp
values or every native BP/display-list effect. KColor must remain an explicit
guest-pointer boundary and hard stop until separately audited and implemented.

## Required validation — pending

- [ ] Execute Nintendo-data-free contracts for all six bridges in headless
  and rendered modes: legal stages/enums, edge values, every argument refusal,
  full-u32 clamp conversion, CPU preservation and null-CPU behavior.
- [ ] Retain each actual bridge/direct trait and native setter through the
  synthetic link and rendered AArch64 syntax gates.
- [ ] Pass all five workflows at the exact candidate revision with recorded
  local or remote provenance.
- [ ] Pass the exact private Rendered Discovery build and record its revision,
  source hashes, dependency pins and NRO size/SHA-256.
- [ ] Transfer that exact NRO and retrieve fresh attributable console reports.
- [ ] Establish return beyond each executed setter family and record the actual
  new frontier and screen observation.

No completed gates, private build or console crossing are claimed by this
candidate record. Host tests with native spies verify forwarding, not real
Aurora register correctness or console pixels. The inspected pinned native
implementation and private rendered link remain separate evidence requirements.
