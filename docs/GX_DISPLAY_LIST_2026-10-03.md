# GX display-list recording candidate — 2026-10-03

The [preceding accepted depth-LOD run](HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md)
returned from the corrected Z24X8 texture LOD, then stopped at
`GXBeginDisplayList` (`0x80172E00`), buffer `0x80394F00`, capacity 16384 bytes.
The user saw a black screen followed by an error. This candidate prepares
Begin and End together. Its [console test](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md)
initially refused guest SU dirty bit 0 before recording. The
[SU-corrected run](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_SPHERE_FRONTIER.md)
subsequently accepts Begin and at least one End return, with broader recording
and replay effects still unaccepted.

## Implementation

The rendered bridges register Begin (`0x80172E00`) and End (`0x80172EB4`).
Begin requires a complete mapped, nonzero, 32-byte-aligned buffer and capacity,
valid non-overlapping GX metadata, a boolean save flag and no unfinished
primitive, buffered parser packet or unhandled guest dirty state. The
[subsequent SU-state correction](GX_SU_STATE_2026-10-04.md) handles guest bit 0
through real native emission and selective shadow publication before snapshots. Host ranges
are compared to reject metadata aliasing through different guest addresses.
Nested recording and End without Begin stop with diagnostics.

Aurora's native `GXBeginDisplayList` flushes pending native registers before
recording. Native writes, translated scalar writes and translated bursts then
use the same checked Aurora buffer and cursor. Bursts bypass the immediate
parser while recording; outside recording they return to the pinned decoder.
The pinned decoder's burst export is renamed for this target to avoid having
two providers of the public burst entry point.

End runs the native dirty-state flush and zero padding, returns the actual
32-byte-rounded count in guest r3 and publishes the guest FIFO cursor/count.
When requested, it restores native GX shadow registers, the guest 0x600-byte
GX context, the HLE vertex/parser state and the producer’s AlphaCompare-valid
flag. Guest GXData+8 retains the SDK's
restoration exception; the recording flag is cleared. Other CPU fields remain
unchanged. Native FIFO-object hardware redirection is not emulated: Aurora's
actual recording transport owns redirection on Switch.

The checked native FIFO header rejects an append **before** it crosses the
buffer boundary, including unsigned cursor corruption. It also diagnoses an
otherwise silently dropped native write. These checks call an unconditional
fatal helper and remain active in release builds. The successful append path
uses `memmove`, allowing an aliased recorded-list burst without memcpy overlap.

CMake creates a private Aurora `lib` source mirror in the build directory,
using [the preparation script](../scripts/prepare-checked-aurora-fifo.py).
The FIFO input is checked against its exact pinned SHA-256; a changed header
requires another audit. Unchanged mirror files keep their timestamps. The
original submodule and its nine-file local integration patch are preserved.

## Validation and limits

The [host contract](../scripts/test-gx-display-list.sh) executes the actual
Switch memory implementation and bridge/transport sources, plus extracted
pinned native Begin/End, AlphaCompare, Flush and FIFO recording/padding bodies. Only allocation,
normal decoder transport and native dirty-register emission are host seams.
ASan, fatal UBSan and LeakSanitizer remain enabled. The original coordinated candidate run passes
335 rendered cases and 30 diagnosed refusals; non-rendered builds pass both
explicit recording refusals. It covers mixed native and
guest writes, empty and full buffers, all padding residues, repeated recording,
both save flags and both initial AlphaCompare validity values, the observed
16 KiB capacity and diagnosed refusals. Expected
abort children disable core dumps and prove rejection before memory, CPU,
recording cursor or native/HLE context mutation. Mirror reuse and changed-input
rejection are also exercised. This is transport/state proof, not rendered-image
proof or a complete native dirty-register decoder test.

`GXBegin` flushes native dirty state before recording its primitive header.
The existing immediate path expands indexed guest attributes into direct
Aurora attributes. Recording those raw indexed bytes needs a separate layout
implementation, so indexed recording remains a diagnosed refusal. Enabled matrix-index
attributes are likewise refused because the immediate bridge omits their native
VCD publication. Pending
guest dirty state also remains unsupported rather than being cleared.

The public build workflow executes the new contract. The synthetic fast-track
workflow retains both bridges and checks their symbols without executing
fabricated recording arguments; the non-rendered bridge explicitly refuses
recording. The rendered syntax gate compiles the real enabled bridge branches.

Local validation, exact-code GitHub workflows and the private rendered build
pass as recorded below. The console must still accept Begin/End and subsequent
commands. Static caller inspection identifies `GXDrawSphere` (`0x80172A30`) and other work beyond Begin;
that address is not `GXSetZTexture`, and its runtime reachability/arguments have
not been measured after this candidate. Recognizable game pixels, full scene
correctness and performance remain unproven.

## Completed validation — 2026-10-04 (Europe/Paris)

Runtime code: `6fb2718b0ab39f217f902f83a97b815a9a1135a2`. All thirteen local
workflow contract scripts pass, with the changed display-list contract rerun on the final sources. It
passes 335 rendered cases, 30 diagnosed refusals and both headless refusals.
Four compiled mutants (missing HLE restore, missing AlphaCompare restore, wrong
count and missing matrix-layout guard) are rejected. Rendered syntax and lint
pass. All five actual GitHub workflows / six jobs pass on this exact code. The [build-switch log](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37157854010)
confirms 335 rendered cases / 30 refusals and both headless refusals.

The final private Rendered Discovery NRO built at `2026-10-03T22:17:32.317144+00:00` in
three incremental tasks using the immutable image
`sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`,
without network access, at `-j3`. It contains 73,465,912 bytes and has
SHA-256 `9c7dfacd4e22bfdca4dfa67b76a1e5c497fef3f20140200af2d7845a265cef4d`.
The earlier complete recording candidate and the preceding depth-LOD NRO
remain preserved privately.

A fresh audit of the actual final link graph scans 226 host inputs, 19 Rust
archives and seven named image libraries. Its eleven scoped names have one
strong provider each: native Init/LOD, Begin/End and AlphaCompare; public and
renamed pinned FIFO bursts; both bridges; the overflow helper; and the
AlphaCompare-valid flag. The checked FIFO header is present in the built
source mirror. This is a scoped ownership check, not a complete audit of the
broader multiple-definition link policy or proof of game pixels.

Dependency pins and the original nine-file integration patch's bytes and
modification timestamps remain preserved. The candidate was subsequently
[transferred and tested on the Switch](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md)
on October 4: its pending-state guard captures guest dirty word `1` before
recording. Begin/End returns and recognizable images remain unproven.
