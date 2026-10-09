# Projection getter returned; scissor-origin boundary — 2026-10-09

The Switch run of PR #343 code `dfd9b50` advances past the projection getter
that stopped the preceding run. A fresh terminal report now diagnoses
`GXSetScissorBoxOffset`, PAL `0x801734E0`, with signed offsets `(0, 0)`, at
59.061 seconds and 648,980 translated dispatches. The exception report is absent.

## Scoped return evidence

The recorded link register `0x8023D630` belongs to the screen setup caller
`0x8023D4E8`. Its local translated control flow reaches the scissor-origin wrapper
`0x80241ACC` after its projection setup on every path. Both alternative projection
helpers (`0x802277A4` and `0x802277D0`) call `0x802417FC`, whose projection getter
is unconditional. The later scissor-origin stop therefore accepts the getter's
return through this checked caller chain. The vector setter in `0x802417FC`
remains conditional; its execution and the returned float contents are not
independently observed on hardware. Its numerical contract remains synthetic.

Both capture controllers have fresh identifiers and report `DISABLED`;
all rate windows have GPU readback off. The 39 retrieved reports contain
17 changed and 22 retained files. The KD open/ioctl/close files have unchanged
content and provide no additional fresh KD evidence. The preceding run already
[accepts that post-resume request](HARDWARE_KD_PROJECTION_2026-10-09.md).
Old SD images/replay files remain preserved and are not attributed to this run.
DBI/MTP readiness is established by the detected USB device and readable reports;
no new visual observation was supplied.

The sampled report records 102 successful presents with zero failures and a
structurally valid FST. Its worker-fiber and OS fields were captured during a
scheduling transition, with the OS running-thread field zero. They are not
claimed as a coherent main-thread snapshot.

| Presentations in window | Duration | Rate |
| --- | --- | --- |
| 44 | 5.070 s | 8.679 Hz |
| 40 | 5.052 s | 7.918 Hz |
| 8 | 11.962 s | 0.669 Hz |
| 6 | 5.371 s | 1.117 Hz |

The measured windows average 3.569 Hz over 27.455 seconds. Different endpoints
and loading/stalls prevent treating this as gameplay FPS or a controlled speedup
comparison. Menu progression and later pixels remain unproven.

## Scissor-origin correction and validation

The existing viewport/depth bridge now implements the same pinned operation as
WiiCompiled `gx_transform.cpp`: forward signed offsets to Aurora, then clear the
guest GXData halfword at +2 when the pointer can be read and written. The native
call precedes this best-effort bookkeeping; a missing/unmapped guest field does
not cancel the native call. Unsigned address wrap and swallowed memory errors
match the pinned wrapper. CPU state is preserved, and headless calls refuse.

The admitted family is the complete nonwrapping interval of the biased 10-bit
half-pixel encoding: each coordinate from −342 through 1705, including odd values
with the pinned quantization. Out-of-range calls refuse before native or memory
mutation. This admits the observed zero pair without tying support to one tuple,
stack address or screen identity. The zero pair emits BP word `0x5902ACAB`.

The existing viewport contract is extended, without another workflow or mutation
suite. ASan/UBSan pass in rendered and headless modes. The real trait/bridge and
Memory slice cover every admitted value on each axis, mixed boundary/odd pairs,
CPU and full-memory canaries, native-before-guest ordering, short/unmapped/zero
GXData, wrapping flag addresses, uninitialized memory, range refusals and native
exceptions. A separate fixture executes the actual pinned Aurora function body
and independently checks all emitted command bytes, register state and canaries
in 12,435 cases. Final private build/provider checks and the console trial remain
pending. Raw reports, game data, private NROs and recovery archives stay local.

The [subsequent trial](HARDWARE_SCISSOR_LIGHT_2026-10-09.md) accepts the observed
scissor helper returning and exposes light-object loading as the next boundary.
