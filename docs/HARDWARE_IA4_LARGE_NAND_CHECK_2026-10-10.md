# Large IA4 load returned; NANDCheck frontier — 2026-10-10

The corrected candidate `5549ab55b97d5afcb3c1811e929d8e92a0154394`
launches via nxlink with exit 0 at 16:17:10 CEST. Its 74,436,664-byte
NRO SHA-256 is
`4a0e1f008d080dbcd8bf30690fc4aef48fb7dda892fbb9ae2aa63aadc3c38862`.
The complete code-HEAD rollup and separate private rendered build pass before
transfer. All 39 durable reports / 850,130 bytes are independently reread,
as is the complete SD NRO. Seventeen reports change and twenty-two are
retained from the preceding Fog-zero trial. Retained files are not fresh
proof of this candidate's effects.

The user first reports continued execution, then clarifies that the screen
is black and the trial is still running. Control response is not reported.
USB diagnostics subsequently establish a durable abort; the earlier visual
observation is not evidence of a running playable game. Capture and frame
dump remain disabled with fresh run identifiers. No fresh Switch image,
playability or controlled performance comparison is claimed.

## Scoped large IA4 return

The fresh texture report records `status=load-pass`, object `0x90D28D70`,
slot 0, data `0x10AA72E0`, 1024 × 1024, IA4 format 2, clamp/clamp,
no mipmap and the full `0x00100000`-byte backing. This is the exact tuple
refused in the previous trial. Production writes this status after native
GXLoadTexObj and guest object publication. The checked `0x805CF598` caller
then continues into drawing; later save-worker execution reaches a distinct
terminal. These facts accept the retained observed load's native return,
not every synthetic size/sampler case or GPU completion.

Discovery records the draw helper at dispatch 654614, setup `0x805CF7E4`
at 654615 and later helper `0x805E7B40` at 654626 on fiber `0x80347498`.
Entry counters alone do not prove a return; the fresh post-load status and
checked caller control flow provide the additional evidence.

## New terminal

The durable DIRECT target is `0x8019EAD0`, attributed to NANDCheck by the
pinned WiiCompiled native override. It stops at dispatch 656321 / 59,286 ms,
guest PC `0x8024373C`, LR `0x8052CA3C`, stack `0x9015AFF8`, fiber
`0x9015B0A0`. Captured arguments are r3 `0xB8` (184), r4 4,
r5 output `0x9015B034`; r6 is `0x10620000` and r7/r8 are zero.
The private checked `0x8052C7E4` caller supplies its stack-local output,
then calls `0x8052B684`, whose wrapper forwards r3/r4/r5 to NANDCheck.
The output word's pre-call contents are not captured and are not inferred.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`
implements virtual NANDCheck by ignoring the first two arguments, validating
four output bytes, writing one zero healthy-result word and returning zero.
Null/unmapped output or a Memory write exception returns -8. This is the
SD-backed virtual NAND contract, not a physical NAND/free-space measurement.
The next bounded candidate must preserve this output and error behavior,
caller context and adjacent memory. No NAND write/create/delete operation
is implied by this frontier.

Fresh diagnostics retain TaskThread reachability and coherent save-worker
fiber/OS identities. The graphics report has no new Dawn uncaptured error.
Five uncontrolled windows contain 102 frames / 52,126 ms (weighted 1.9568
presents/second). Recorded presents and FIFO work do not establish visible
pixels. Private callers, game payloads, NROs and raw archives remain excluded.
