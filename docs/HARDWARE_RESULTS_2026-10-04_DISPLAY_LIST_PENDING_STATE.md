# Hardware result — display-list pending texture state (2026-10-04)

The [coordinated display-list candidate](GX_DISPLAY_LIST_2026-10-03.md), runtime
code `6fb2718b0ab39f217f902f83a97b815a9a1135a2`, was tested on the Switch.
Its private Rendered Discovery NRO contains 73,465,912 bytes, SHA-256
`9c7dfacd4e22bfdca4dfa67b76a1e5c497fef3f20140200af2d7845a265cef4d`.
All five exact-code workflows / six jobs, the private build, rendered syntax,
335 rendered contract cases / 30 refusals, both headless refusals and four
mutation checks passed before launch. Documentation commit `aab8e8d` changes
only Markdown. Launch checks reverified the artifact, non-document source
hashes, dependency pins and preserved integration patch.

Fresh UDP discovery received `bootnx` from `192.168.1.194:28280` at
**09:18:57 UTC** without consuming the TCP listener. Nxlink transferred the
exact NRO with **exit 0 at 2026-10-04 09:19:26.425033 UTC** (11:19:26
Europe/Paris): 26,725,824 compressed bytes, 2247 blocks. Its displayed 36.38%
is a compression ratio, not incomplete transfer progress.

USB/MTP retrieval at **09:21:55 UTC** preserved **29 reports / 534,803 bytes**.
Thirteen changed versus the preceding depth-LOD run; sixteen are identical
and cannot independently be dated. Source timestamps are unavailable. All
manifest sizes, SHA-256 hashes, baseline deltas, full ZIP CRC and archived
raw bytes were independently verified. The new display-list report, changed
blocker and coherent discovery/heartbeat cohort identify this candidate run.
Raw files remain private under
`.deps/network-tests/gx-display-list/runs/20261004T092155Z/`.

The visual observation and any on-screen error code are pending the user's
reply. The durable report independently establishes an intentional abort
at the pending-state guard. It does not identify the console's displayed
error text or establish recognizable game pixels.

## Accepted diagnostic boundary

The bridge arrived at `GXBeginDisplayList` and refused before native recording:

| Field | Value |
| --- | --- |
| Reason | `GX_DISPLAY_LIST_PENDING_STATE` |
| Target | `0x80172E00` |
| Dispatch / elapsed | 606826 / 102408 ms |
| LR / stack | `0x8021A568` / `0x80394E40` |
| Buffer / capacity argument | `0x80394F00` / `0x4000` (16 KiB) |
| Guest GXData | `0x803437C0` |
| Guest dirty state | `0x00000001` |
| Guest recording / save-context flags | 0 / 1 |
| Native recording / capacity / cursor | inactive / 0 / 0 |
| HLE unfinished primitive / buffered bytes | 0 / 0 |

First-hit discovery captures the preceding texture constructor `0x8021A4D8`
at dispatch 606803, then `0x8021B6A4` at 606824 and Begin at 606826. Their
shared thread context agrees with the earlier accepted caller path. The LOD
status file is byte-identical to the preceding run; it is not newly dated by
itself. Fresh later caller progression supports continued traversal of that
path without establishing a new dedicated LOD return trace.

The native Begin call, guest-context save, buffer redirection and `begin-pass`
status are all after this guard in the implemented bridge. Therefore **Begin
did not return, no native list recording began and End was not reached**.
The implementation's host tests remain valid within their declared scope;
the hardware result exposes a pending-state case they intentionally refuse.

## Cause and next implementation boundary

The pinned Aurora `__GXData_struct` and `__GXSetDirtyState` identify bit 0 as
SU texture size/bias state. The texture-load bridge explicitly publishes
this bit in guest GXData after native `GXLoadTexObj`. This is a known local
producer of the observed state, not a trace proving its most recent writer.
The native dirty-state routine derives SU registers from texture dimensions,
wrap modes, active TEV/indirect stages and manual-scale flags before clearing
its native dirty word.

The current Begin bridge has no equivalent guest-state synchronization and
refuses every nonzero guest dirty word. The next fix must reconcile this
texture-state publication with native flushing and guest shadow/context
semantics before recording, with executable contracts for the actual SU
emissions and preservation effects. Clearing the bit without applying or
proving the corresponding state would not establish a correct port.
Other dirty bits, indexed/matrix-index recording, later replay and the
statically identified `GXDrawSphere` remain separate unaccepted boundaries.

## Liveness and rendering limits

The watchdog records 99 samples: 97 ACTIVE and two recovered STALE samples,
with a maximum sampling interval of 1399 ms. The preceding heartbeat at
606083 records 1556 guest FIFO writes; the later post-main snapshot at 606537
records 1558, last word `E8000156`. Both retain 99 successful presents / zero
failures, 3826 StaticR dispatches, six TaskThread runs and a valid 64,224-byte
FST. These counters precede Begin and do not establish visible content after
the texture constructor. The graphics report is unchanged from the baseline.

No recording overflow is reported by the new display-list diagnostic;
refusal occurs before recording. This is not exhaustive native-exception
proof. Begin/End returns, recognizable game pixels, sustained execution,
input/audio correctness and representative-scene performance remain open.
