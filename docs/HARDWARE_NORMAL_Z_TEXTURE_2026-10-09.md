# Normal matrix returned; depth-texture stop — 2026-10-09

The trial of normal-matrix candidate `cce8d53` records its `GXLoadNrmMtxImm`
entry and subsequent calls with the normal-matrix stage. It then deliberately
aborts at the unsupported direct call `GXSetZTexture`, PAL `0x801720C0`, after
56.409 seconds. The recorded arguments are `GX_ZT_DISABLE`, `GX_TF_Z8`, bias 0.
This accepts the observed matrix return, rather than establishing arbitrary
matrix or pixel correctness on hardware.

Both capture controllers have fresh identifiers and report `DISABLED`; no old
images or FIFO files are attributed. Of 39 retrieved reports, 16 differ from
the preceding run and 23 remain unchanged. No exception report is present.
The operator reports the program running after launch, then USB readiness;
there is no additional screen or fluidity observation for this trial.

The four presentation windows range from 0.728 to 8.483 Hz and average 3.306 Hz
over 29.950 seconds, including loading and stalls. These are neither steady
play FPS nor a controlled speed comparison. The sampled main-thread snapshot
is coherent, has a valid FST and records 102 successful presents. Those counters
do not establish new visible pixels.

## Depth-texture bridge

The source-owned missing-direct registry routes `GXSetZTexture` without editing
the shared ABI header or generated translations. The bridge supports the three
GX operations: disable, add and replace. It rejects full operation words outside
that enum before any native output. Headless execution diagnoses the missing
renderer. CPU state and guest memory are preserved.

The pinned native writer encodes Z8 as 0, Z16 as 1 and every other full format
word as 2. The bridge normalizes that default to a valid Z24X8 enum before the
native call, preserving the wire semantics without constructing invalid C++
enum values. Bias remains a full 32-bit ABI input; native GX keeps its low
24 bits. Native output consists of the F4 bias and F5 operation/format BP writes
and sets the existing `bpSent` flag. The pinned command processor decodes both
registers and invalidates pipeline state; the pinned shader already implements
the add/replace paths. The bridge adds no shader or pipeline behavior.

The existing GX extension suite passes 7,830 complete BP-packet cases across all
operations, canonical/default/wide format words, bias boundaries and prior BP
flag values. It executes the actual pinned native writer, complete native GX
state struct, BP decode cases and dirty-state helper. CPU, guest-memory and
native-state canaries catch unintended changes; refused calls must leave output
and native state untouched. The decoder runs against a small state/epoch seam,
so this does not validate a GPU pipeline or depth pixels.

The light and normal contracts still execute their actual native bodies; common
fixture declarations are shared among the three tests. Their existing 864 light
and 1,008 matrix packets, registry priority, probe and refusal checks pass.
Merging requires the private rendered build, SDK syntax, scoped provider and
retention audits, and every published exact-HEAD CI check to succeed. Depth-
texture return and later screen progression remain pending hardware results.

Raw diagnostic reports, game data and private NROs remain local.
