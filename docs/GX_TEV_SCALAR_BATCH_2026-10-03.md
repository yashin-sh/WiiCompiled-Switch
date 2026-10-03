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
Their presence is implementation evidence; validation and bounded hardware
acceptance are recorded below.

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

## Static caller and hardware acceptance basis

After GXSetNumTevStages(1), `func_80241380` loops over all s=0..15:

| Order | Caller call / arguments |
| --- | --- |
| 1 | Direct(s) |
| 2 | Existing Order(s,255,255,255) |
| 3 | ColorIn(s,15,15,15,15) |
| 4 | ColorOp(s,0,0,0,1,0) |
| 5 | AlphaIn(s,7,7,7,7) |
| 6 | AlphaOp(s,0,0,0,1,0) |
| 7 | SwapMode(s,0,0) |

This is 96 new-setter calls plus 16 existing Order calls. Static call order
alone did not accept the loop; the fresh hardware result below adds the
required later frontier. The forecast first missing boundary, KColor
`0x80171ED4`, is now captured with ID 0, pointer r1+20 = `0x80398FCC`,
LR `0x80240F98`, on the same guest fiber and nested stack. Its four RGBA
bytes come from guest small-data memory and remain unknown; none are
assumed or fabricated here.

Without callback dispatches, a first mapped Direct entry at count D would
be followed by the KColor unknown-DIRECT record at D+111. Known calls count
before invocation; an unknown DIRECT dispatch does not add another increment. Polling
can add callbacks and change diagnostic stages. Assess actual arguments,
LR/stack/fiber, executed control flow and later durable state together,
rather than treating an exact counter delta as unconditional.

The fresh KColor boundary matching this path establishes return of all
16 iterations and the six setters on these caller tuples. First-hit tracing
still does not contain 96 individual return records. It does not
hardware-validate alternate inputs, comparison operations, noncanonical clamp
values or every native BP/display-list effect. KColor must remain an explicit
guest-pointer boundary and hard stop until separately audited and implemented.

## Completed validation and bounded hardware acceptance

Code candidate `549ef801e1948b65c0ce47d9dbd6417c85d1f69d` passed:

- all 10 lint steps and 28 build/verification steps replayed locally from the
  five workflow definitions; remote validation was completed later on the
  integrated revision recorded below;
- all nine executable host contracts, with ASan/UBSan fatal and LSan active;
- the new TEV contract in rendered=0/1: 55,568 valid calls and 246 diagnosed
  SIGABRT refusals per mode, including CPU bytes checked after termination;
- synthetic link retention of all six bridges and direct traits;
- the Nintendo-data-free AArch64 rendered HLE/Discovery syntax gate;
- the exact private Rendered Discovery build, flags ON/ON, network disabled,
  three jobs, immutable image `sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`.

The new local NRO is
`.deps/network-tests/gx-tev-scalar-batch/WiiCompiled-Switch-tev-scalar-rendered-discovery.nro`,
73,310,264 bytes, SHA-256
`cc88a78caf2583332f277f3544bdc3013d45e8572630850a8b16dde57e701ffb`.
The new ELF retains 19 checked strong text symbols: six TEV bridges, six
TEV native setters, the six existing coordinate bridge/native symbols and
libnx exception handler. All tracked source hashes remained unchanged during
this build. WiiCompiled, Dawn and Mesa pins, and the existing WiiCompiled
integration patch bytes/modification times were preserved. Previous hardware
NROs remain intact. Detailed validation, source hashes and logs remain local
beside this artifact.

The targeted post-build provider audit checked 219 actual host link inputs
and all six new objects: each object defines its mkw_switch_hle_* bridge
and references the expected GX native symbol. There is exactly one expected
Aurora provider per native TEV name: Direct from GXBump.o, the other five
from GXTev.o, in libm3_aurora_gx.a. The earlier scan of 19 Rust archives and
seven container -l libraries was explicitly reused after matching input
paths/names and the immutable build image; it is not a fresh container nm run.
The ignored audit JSON distinguishes these evidence phases and verifies the
candidate source hashes and NRO. This does not establish general ownership
of other symbols under the private link's broad allow-multiple-definition option.

- [x] All five GitHub workflows passed on integrated revision `e76e8f38`,
  including both build jobs; the private build reproduced the same NRO digest.
- [x] Transfer this exact NRO and retrieve fresh attributable console reports.
- [x] Establish return beyond each executed setter family and record the actual
  new frontier and screen observation.

The [accepted hardware run](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md) transferred with exit 0 at
2026-10-03 11:39:21 UTC. Its 28 verified reports, 528,821 bytes, include
eleven changed files against the audit baseline. All six first-hit tuples,
coherent caller/stack/fiber and the later KColor boundary establish return
from all sixteen iterations: 96 new setter calls plus 16 existing Order calls.
Direct first hit is 604927; KColor ID 0, pointer `0x80398FCC`, is reached at
605056 / 98.265 seconds, stage SwapMode. The +129 delta exceeds the
callback-free +111 and does not independently count callback events.

The user reported a black screen and an error at the end. All 95 watchdog
samples are ACTIVE. The changed snapshot at 604804 still precedes the loop
and retains 1556 guest FIFO writes / 99 successful presents / 0 failures.
It does not measure native TEV commands or prove pixels. Alternate legal
inputs and malformed-input refusals retain host evidence only. Host forwarding
sinks do not verify Aurora BP decoding or console pixels. KColor is arrived
at, not returned; its RGBA bytes are unknown and it remains a hard stop.
