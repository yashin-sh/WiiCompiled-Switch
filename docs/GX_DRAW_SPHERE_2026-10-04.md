# Bounded GXDrawSphere recording bridge — 2026-10-04

The [preceding console run](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_SPHERE_FRONTIER.md)
returned from Begin and at least one End, then stopped at GXDrawSphere
`0x80172A30`, parameters `(4,8)`. This candidate connects that call to the
existing pinned Aurora SDK routine. The second audited constructor variant
`(8,16)` is included. A [fresh console run](HARDWARE_RESULTS_2026-10-04_SPHERE_PAD_READ_FRONTIER.md)
now accepts both variants returning and reaches PADRead.

## Implementation

The bridge accepts only those full-word argument pairs while the coordinated
native/guest display-list recording is active. It rejects changed context,
pending guest state, unfinished HLE primitives, native dirty bits outside
VCD/VAT and insufficient remaining capacity before native writes. It reserves
geometry bytes, strip headers and 512 bytes for state emission, restoration
and final alignment. Every native write still uses the checked FIFO.

The actual native routine temporarily selects direct XYZ float positions and
normals in format 3, optionally emits direct ST float texture coordinates,
draws latitude strips, then restores vertex descriptors and format. Guest
GX shadows, HLE state and CPU registers are preserved; the shared recording
cursor advances. A durable `fast-track-gx-sphere.txt` records successful return,
arguments and the actual byte delta. This bridge requires the rendered build;
headless use is explicitly refused.

Host execution uncovered undefined behavior in the pinned vertex writer:
`scalar_f32(-1)` eagerly converted the coordinate to `uint32_t`. The float-kind
consumers use only its float member. The build mirror initializes the unused
integer members to zero, preserving float and quantization behavior. The
preparation script checks the complete original GXVert.cpp SHA-256
`0f1f91b132507547a65a34c6acc7b4425748ca5ea5542360270e8908ea6eaecc` and refuses
changed inputs. Original upstream files are untouched and repeated preparation
preserves unchanged mirror timestamps.

## Executable evidence

`bash scripts/test-gx-display-list.sh` passes **648 rendered cases and 48
refusals**, plus three headless refusals, under ASan, fatal UBSan with explicit
float-cast-overflow checking and LeakSanitizer. It executes pinned sphere math,
vertex getters/setters, VCD/VAT writes, the complete vertex writer, native
Begin/End and the coordinated bridge. Renderer-cache comparison and logging
are host seams; GPU decoding and presenting are outside this contract.

The sixteen new success cases cover both subdivision pairs, all four incoming
TEX0 descriptor modes and both save flags. An independent packet parser checks
strip/vertex counts, every position, normal and optional UV, unit lengths,
restored descriptors/formats, CPU/guest/HLE preservation, shared cursor,
End padding and buffer sentinels. Guard cases include zero, truncating and
unsupported arguments, dirty state, inactive recording and insufficient
capacity; refusal must leave CPU, guest, native/HLE and FIFO state untouched.
The same suite with the original float constructor fails at the real writer's
negative-to-unsigned conversion, establishing a regression check.

All thirteen local workflow contract scripts, rendered AArch64 syntax, lint
and 440 local Markdown links pass. Four compiled sphere mutants are rejected.
All five exact-code GitHub workflows / six jobs pass on
`e7dd680603ba4c9d00fc673c12307ef099f3e0c0`; the [actual build-switch log](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37195265760)
confirms both new contract counts. The private Rendered Discovery build also
passes, with 45 retained strong symbols and sixteen scoped unique providers
across 226 host inputs, 19 Rust archives and seven named image libraries.
The mirrored FIFO and corrected vertex writer were independently verified.

The NRO is **73,478,200 bytes**, SHA-256
`e2b0c3f283a0cf2e6e0b2c6bc15a6d431d0010ece269e1abcdb5ee09ead3ea50`. The immutable local image ran
without network access. An initial three-task build was intentionally
interrupted and resumed at six tasks after resource measurement; completed
objects were retained. Pins and the original submodule patch/mtimes remain
unchanged. The NRO and raw build/diagnostic files stay private. Both sphere variants now return on console; display-list replay and
recognizable game pixels remain unaccepted. Native GXEnd's pinned
size accessor measures the live FIFO only; the independent parser above checks
recorded vertex payload rather than relying on that native size check.
