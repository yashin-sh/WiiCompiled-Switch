# GX display-list recording candidate — 2026-10-03

The [latest accepted console run](HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md)
returned from the corrected Z24X8 texture LOD, then stopped at
`GXBeginDisplayList` (`0x80172E00`), buffer `0x80394F00`, capacity 16384 bytes.
The user saw a black screen followed by an error. This candidate prepares
Begin and End together; neither return has yet been accepted on hardware.

## Implementation

The rendered bridges register Begin (`0x80172E00`) and End (`0x80172EB4`).
Begin requires a complete mapped, nonzero, 32-byte-aligned buffer and capacity,
valid non-overlapping GX metadata, a boolean save flag and no unfinished
primitive, buffered parser packet or unhandled guest dirty state. Host ranges
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
GX context and the HLE vertex/parser state. Guest GXData+8 retains the SDK's
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
pinned native Begin/End and FIFO recording/padding bodies. Only allocation,
normal decoder transport and native dirty-register emission are host seams.
ASan, fatal UBSan and LeakSanitizer remain enabled. The final local run passes
331 rendered cases and 29 diagnosed refusals; non-rendered builds pass both
explicit recording refusals. It covers mixed native and
guest writes, empty and full buffers, all padding residues, repeated recording,
both save flags, the observed 16 KiB capacity and diagnosed refusals. Expected
abort children disable core dumps and prove rejection before memory, CPU,
recording cursor or native/HLE context mutation. Mirror reuse and changed-input
rejection are also exercised. This is transport/state proof, not rendered-image
proof or a complete native dirty-register decoder test.

`GXBegin` flushes native dirty state before recording its primitive header.
The existing immediate path expands indexed guest attributes into direct
Aurora attributes. Recording those raw indexed bytes needs a separate layout
implementation, so indexed recording remains a diagnosed refusal. Pending
guest dirty state also remains unsupported rather than being cleared.

The public build workflow executes the new contract. The synthetic fast-track
workflow retains both bridges and checks their symbols without executing
fabricated recording arguments; the non-rendered bridge explicitly refuses
recording. The rendered syntax gate compiles the real enabled bridge branches.

Local validation and the private rendered build are being completed. The
console must still accept Begin/End and subsequent commands. Static caller
inspection identifies `GXDrawSphere` (`0x80172A30`) and other work beyond Begin;
that address is not `GXSetZTexture`, and its runtime reachability/arguments have
not been measured after this candidate. Recognizable game pixels, full scene
correctness and performance remain unproven.
