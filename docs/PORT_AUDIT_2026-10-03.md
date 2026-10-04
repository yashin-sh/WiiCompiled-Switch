# Port, scripts and CI audit — 2026-10-03

## Assessment and scope

The architecture and evidence-driven bring-up method are coherent, but this
is an incomplete experimental port. Neither passing CI nor this audit proves
bug-free code, full Wii compatibility, recognizable game pixels or playable
performance. The [audit hardware result](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md)
for code `b3484117` / NRO `7ecbc8a9…` preserves the eight exact disabled
coordinate triples and reaches the same GXSetTevDirect `0x80171B58`, stage 0.
It accepts normal-path non-regression only. Direct had not returned in that
audit run, and the user again saw black output. SIZE_MAX, Present(false),
teardown exceptions and shutdown recovery were not exercised by this console run.

The audit reviewed all five workflow definitions, the 31 existing shell/Python
scripts and the current documentation. Manual runtime review focused on memory,
dispatch/ABI, cooperative context switching and rendered frame lifecycle;
automated gates cover the wider compiled port. This is not a line-by-line
proof of every translated game function or every native subsystem.

## Corrected findings

| Area | Defect and resulting behavior |
| --- | --- |
| Memory | `start + length` could wrap in uint64_t and return a pointer inside a tiny allocation for SIZE_MAX bytes. Bounds now use subtraction before addition; the new executable contract failed before the fix. |
| GuestFlat initialization | The analogous guest/backing range additions now reject overflowing sizes before allocation and page rounding. |
| Host context | Stack alignment could wrap for near-SIZE_MAX sizes. The allocation boundary is reviewed and tested separately from the AArch64 context switch. |
| Presentation | After Aurora completed a frame, a false Present result skipped cleanup and retained an active-frame flag for consumed buffers. Cleanup now precedes the success/failure branch. |
| Renderer diagnostics | Teardown exceptions could be followed by RESULT=PASS. The final result now reflects that failure. |
| Rendered syntax gate | Restoring pinned files before/after compilation overwrote pre-existing edits. The gate reuses an applied patch, refuses conflicting edits and only undoes its own temporary patch. |
| Host tests | Four GX scripts allowed UBSan recovery and could print PASS after a runtime error. All executable contracts now make sanitizer errors fatal. |
| Shell command output | DTK follow-up commands used JSON quoting, which did not protect shell substitutions in paths. They now use shell quoting with a regression case. |
| Forecasting | UNPROVEN_ARGS bridges were not consistently classified as constrained native mappings. The scanner now recognizes the UNPROVEN family and has a self-test. |
| Diagnostic bundles | Repeated reads could produce inconsistent report/hash/raw bytes, and interrupted writes could replace a valid bundle. Inputs are captured once; ZIP metadata is deterministic and output replacement atomic; input/output collisions are rejected. |
| PPC scanning | The texture scanner now checks ELF ISA and guest-address range before interpreting instructions; invalid CLI addresses fail cleanly. |
| Build preparation | Already present pins/patches and identical probe source are reused; rendered preparation carries its intended mode instead of toggling OFF then ON and building an unrelated triangle probe. Cleanup and ambiguous output selection have explicit failure boundaries. |
| CI | Main pushes were excluded; permissions, timeouts and concurrency were incomplete; action tags floated; PR checkout differed from the exact-HEAD policy. All five workflows now check the candidate HEAD, include main, use read-only tokens and immutable action revisions, and bound/cancel overlapping work. Actionlint's archive is checksum-verified before extraction. |
| Formatting gate | Failure to resolve a diff base could become an empty-file PASS. The base is checked and the merge-base used before selecting candidate changes. |
| Documentation | September snapshots/frontiers were presented as current. README, roadmap, index and current guides now distinguish the accepted coordinate baseline, audit normal-path non-regression, bounded TEV scalar batch and evidence limits. |

The runtime fixes do not expand guest dispatch coverage or claim a new
hardware crossing. The presentation-failure path has been attributed to the
pinned Aurora lifecycle, not reproduced as a failure on the console. The host
context allocation test does not execute the handwritten AArch64 switch.

## GitHub configuration and reproducibility

The last five remote workflow runs inspected were successful on `e8f870b…`,
before these local audit changes. The [recorded build run](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/36986236474)
is historical evidence, not validation of the new candidate.

The active rules for main require a PR and protect against deletion/force push.
They do **not** contain required status checks. Before relying on CI as a merge
gate, require the six actual job contexts: `lint`, `synthetic-fast-track`,
`synthetic-bootstrap-prelude`, `synthetic-sequence`, `gx-contracts` and `nro`.
This audit reads that configuration; it does not change repository rules.

Actions retain their v4 major versions and are pinned to official commit SHAs;
credentials are not persisted. This follows GitHub's
[security guidance](https://docs.github.com/en/actions/reference/security/secure-use).
Concurrency saves overlapping push/PR runs on the same source branch, with
fork/tag separation; sequential duplicate events can still run. See
[GitHub concurrency semantics](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency).

`devkitpro/devkita64:latest`, Ubuntu and apt-provided compilers still float.
The public registry resolved the devkitPro image to
`sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`
during this audit, but that exact container has not been validated here.
Do not claim a reproducible toolchain until a selected digest and host-tool
versions have passed the required gates. Actionlint's checksum is pinned from
the [official release manifest](https://github.com/rhysd/actionlint/releases/download/v1.7.12/actionlint_1.7.12_checksums.txt).

## Validation and outstanding work

The code candidate `b3484117f5923d72a04d860eecd702452157cdde` passed the final local gates:

- all 10 lint/quality steps and 28 executable build/verification steps from
  the five workflow definitions; local setup substitutions are recorded in
  `.deps/network-tests/audit-2026-10-03/validation.json`;
- all eight executable host contracts under fatal ASan/UBSan, including six
  GX contracts in both rendered modes and the memory/context boundaries;
- all 21 build-script fixtures, plus current documentation-link checks;
- the Nintendo-data-free AArch64 rendered HLE/Discovery syntax gate;
- the complete private Rendered Discovery NRO build, with both intended flags
  ON and the existing WiiCompiled/Dawn/Mesa pins preserved.

The private build used the immutable local image
`sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`
with network disabled. Its separate local artifact is
`.deps/network-tests/audit-2026-10-03/WiiCompiled-Switch-audit-rendered-discovery.nro`,
73,297,976 bytes, SHA-256
`7ecbc8a9fe1efb31697c2ee36d0b0b648a8e87d3fa7dda1fb9d262d6de5b7d09`.
The rendered ELF retains the three coordinate HLE bridges, their three native
Aurora setters and the libnx exception handler as strong text symbols. Build
metadata and logs are retained locally beside that artifact. The nine-file
WiiCompiled integration patch and its modification times remain unchanged.

No GitHub Actions run was started for this local candidate. The audit NRO
has now run on Switch: transfer exit 0 at 2026-10-03 09:25:19 UTC and fresh
reports accept normal-path non-regression through the same TEV Direct frontier.
The earlier coordinate result remains its baseline; it was not reused as
proof that the audit binary ran.
Public synthetic NRO builds and nm assertions
prove compilation/link retention; they do not execute NROs on Switch.
Executable host contracts cover their stated argument and memory domains,
not every resource/scheduler/audio/input behavior.

The private rendered link's broad `--allow-multiple-definition` policy still
deserves an explicit symbol ownership audit; successful linking does not prove
that every selected definition is the intended one. Exact-state HLE guards and
large texture-descriptor tables
are useful bring-up boundaries but need contract-based generalization before
broader compatibility can be claimed.

Renderer error handling also needs broader fault-injection coverage: a partial
GXCopyDisp exception still returns without a phase-specific recovery contract,
and an outer frame-begin retry deadline cannot interrupt the pinned Aurora
staging-map wait. No retrieved hardware report establishes either failure.
The display-list writer also inherits the pin's write-before-wrap ordering:
a write after an exact fill can cross the list's logical capacity while still
being inside mapped guest RAM. That separate stateful recording contract needs
hardening and targeted tests before its overflow path is accepted.

The GCC return normalizer is scoped to the pinned emitter's output forms.
Its regex transformation is not a general C++ parser: arbitrary string/lambda
forms can be rewritten incorrectly. A read-only review of the 106 current
generated C++ shards, including the
72 targeted base_common files and 507 typed statefree returns, found no
problematic active-body literal/lambda match or braced return left to rewrite.
That current-input check does not establish safety for arbitrary future forms.
The headless incremental helper also keys its cache
on HEAD, so documentation-only commits can trigger a full rebuild; content-based
cache identities remain an optimization task.

Coordinate retrieval at 2026-10-03 09:03:40 UTC preserved 28 reports,
526,932 bytes, with 12 changed from the matrix baseline and verified hashes/
ZIP CRC. They accept `Gen2(c,1,4,60,0,125)`, `Scale(c,0,0,0)` and `Bias(c,0,0)`
for c=0..7; enabled branches and arbitrary sizes remain host-tested only.
Caller `0x80241380` at dispatch 605620 captures the restored stack, final
coordinate 7 and Bias stage. Direct stage 0 then blocks at 605633, 100,205 ms.
The +35/+13 dispatch deltas versus callback-free +23/+1 are compatible with
VI polling, without an exact callback count. Snapshot 605367 precedes the
loop: 1556 FIFO writes, 99 successful presents / 0 failures. Watchdog history
has 94 ACTIVE samples and one recovered one-second STALE interval. These
counters do not measure later native FIFO work or establish visible pixels.

Audit retrieval at 2026-10-03 09:32:30 UTC preserved 28 reports, 528,058
bytes, with 12 changed from the coordinate baseline and verified hashes/ZIP
CRC. Scale at 608333 and Bias at 608346 precede the restored caller at
608374; Direct stage 0 then blocks at 608381, 107,925 ms. The +41/+7 deltas
versus callback-free +23/+1 are compatible with VI polling, without an exact
callback count. Snapshot 608109 precedes the loop and retains 1556 FIFO
writes, 99 successful presents / 0 failures, valid FST and coherent guest
identities. Watchdog history has 101 ACTIVE and one recovered STALE sample.
The user confirmed black output. This accepts the audit binary's ordinary
executed path; SIZE_MAX rejection, Present(false), teardown exceptions and
shutdown/error-recovery paths remain unexercised on hardware.

The subsequent [TEV scalar batch](GX_TEV_SCALAR_BATCH_2026-10-03.md) adds
a ninth executable contract beyond the eight audited above. Its local gates,
all five GitHub workflows on integrated code `e76e8f38`, private build and
[fresh console result](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md) passed. NRO `cc88a78c...`
returns from six setters on default tuples across stages 0..15, then stops at
KColor ID 0, pointer `0x80398FCC`, dispatch 605056 / 98.265 seconds. The
user reported black output and an error at exit. Alternate scalar arguments
retain host contracts only; KColor was that scalar run's arrival boundary.
The [TEV color/table batch](GX_TEV_COLOR_BATCH_2026-10-03.md) passed all five
GitHub workflows and its exact private build (code `1333b0e2`, NRO `a56be881...`).
Its [fresh console result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
now establishes all twelve calls returned, with AlphaCompare `0x80172088`
as that run's arrival boundary. The separate
[AlphaCompare candidate](GX_ALPHA_COMPARE_2026-10-03.md) preserves the pinned
native forwarding and existing host validity flag. Its local contracts, all
five exact-code GitHub workflows and private Rendered Discovery build pass;
its subsequent [console run](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
accepts AlphaCompare returned on (7,0,0,7,0), through existing ZMode to Fog.
Fog type 0, four f64 parameters and readable RGBA 255,255,255,255 are captured.
That preceding run stopped before Fog returned; the user confirmed black output
and an error. The [later Fog/ZCompLoc result](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
now accepts both new bridge returns and the existing pixel setup. Native init
of a 4×4 depth texture passed; GXInitTexObjLOD rejects its valid format 22
because the structural layout table lacks it in that preceding candidate.
The subsequent [depth-LOD hardware result](HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md)
accepts the corrected LOD return and identifies GXBeginDisplayList as the new
DIRECT boundary. The user confirms black output followed by an error;
recognizable game pixels remain unproven. The [coordinated display-list candidate](GX_DISPLAY_LIST_2026-10-03.md)
now implements a shared bounded native/guest recording transport and context
restoration; this implementation still needs console acceptance.
Input mapping, audio output, remaining Wii services, pixel correctness and
representative-scene profiling are still open. See the updated
[`ARCHITECTURE.md`](ARCHITECTURE.md) and
[`FAST_TRACK_VALIDATION_POLICY.md`](FAST_TRACK_VALIDATION_POLICY.md).


## Earlier console result — TEV colors crossed (2026-10-03)

The [fresh color/table hardware result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
supersedes the earlier pending color/table status. All twelve executed calls
returned through the coherent later caller; AlphaCompare `0x80172088`,
(7,0,0,7,0), is the new DIRECT hard stop. Black output and a crash persist.
Actual RGBA bytes and recognizable game pixels remain unproven. The elapsed
time includes an unexplained watchdog sampling gap, so it is not a performance
measurement. Prior dated results above retain their original scope.


## Earlier console result — AlphaCompare crossed (2026-10-03)

The [fresh AlphaCompare hardware result](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
establishes its observed tuple returned. Fog `0x801722CC` is the new DIRECT
hard stop at dispatch 603961 / 99.156 seconds, with actual float parameter bits
and readable color captured. All 96 watchdog samples are ACTIVE. Preceding
present counters do not prove visible pixels. The user confirms a black screen
followed by an error; the exact on-screen wording is unavailable. Earlier dated
sections retain their original scope.


## Earlier console result — Fog/ZCompLoc crossed (2026-10-03)

The [fresh hardware result](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
establishes the admitted Fog call, ZCompLoc(1) and existing pixel setup returned.
The next stop is GXInitTexObjLOD at `0x80170A4C`, dispatch 609384 / 109.572
seconds. Native init passed for the 4×4 `GX_TF_Z24X8` object; the LOD layout
validator lacks full format 22. Its forwarding and later drawing remain
unproven. The later snapshot records 1558 guest FIFO writes and the same 99
successful presents; those counters do not establish a visible frame. Current
screen observation is pending. Earlier dated sections retain their scope.


## Earlier console result — depth LOD crossed (2026-10-03)

The [fresh hardware result](HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md)
establishes LOD returned on the observed depth object with `lod-pass` and guest
word0 `0x105`, then reaches GXBeginDisplayList `0x80172E00`: buffer
`0x80394F00`, capacity 16 KiB, dispatch 609010 / 108.440 seconds. Begin has
not returned; recording/replay require coordinated FIFO/context/buffer work.
The user confirms black output and an error. The snapshot before the texture
constructor retains 99 successful presents, without proof of visible pixels.
Earlier dated records retain their scope.

## Earlier console result — pending SU texture state (2026-10-04)

The [display-list hardware result](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md)
records an exact validated NRO transfer with exit 0, 29 verified reports and
`GX_DISPLAY_LIST_PENDING_STATE` at Begin (606826 / 102.408 seconds). Guest
dirty word `1` is pending SU texture size/bias state; native recording is
inactive. Begin did not return and End was not reached. The next implementation
must reconcile native flushing and guest texture shadow/context semantics.
The guard is diagnosed, not hardware acceptance of recording. Screen
observation is pending; recognizable game pixels remain unproven.

## Earlier console result — Begin/End return, Sphere boundary (2026-10-04)

The [verified display-list result](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_SPHERE_FRONTIER.md)
accepts native empty-update SU processing and Begin/End return. A later End
caller allocates 64 bytes and another recording begins. GXDrawSphere `(4,8)`
is the new DIRECT stop, 607503 / 104.178 seconds. Thirty reports are verified;
nonempty SU emissions, broader recording/replay effects, sphere return and
recognizable game pixels remain unaccepted. Visual observation is pending.

## Latest console result — sphere return, PADRead boundary (2026-10-04)

The [verified sphere result](HARDWARE_RESULTS_2026-10-04_SPHERE_PAD_READ_FRONTIER.md)
accepts both `(4,8)` and `(8,16)` returning. Thirty-one reports / 541,214 bytes
are checked against their baseline and archive. PADRead `0x801AF44C` is the
DIRECT boundary at dispatch 617831 / 110.266 seconds. The user confirms black
output followed by an error. The snapshot advances to 3,937 guest FIFO writes
and retains 99 preceding successful presents, with zero list replay calls.
Input bridging, actual replay and recognizable game pixels remain open.
