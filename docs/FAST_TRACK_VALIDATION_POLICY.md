# Fast-track validation policy

This document defines the validation contract for blocker-driven RMCP01 bring-up
on real Nintendo Switch hardware.

The 2026-09-20 audit established a hardware-first strategy: attribute the first
exact blocker against pinned WiiCompiled, preserve its semantics, validate, and
retest on hardware. On 2026-10-02, the user authorized bounded audited GX batches
to reduce repeated builds and console round trips, as described in
`RMCP01_DISCOVERY_SCAN.md`. A batch changes implementation scope, not the proof
required to call each boundary hardware-crossed.

The audit also identified two places where the proof standard must be stricter:
public CI does not build the private RMCP01 rendered target, and a dispatch
counter alone does not prove that a native bridge executed successfully and
returned.

## Validation ladder

Every blocker-driven change should pass these stages in order:

1. **Exact hardware evidence**
   - capture the first unsupported dispatch, attributable host exception, or
     durable stall;
   - record the exact target/stage and the registers actually present in the
     diagnostic;
   - never invent unrecorded register values or guest-memory contents.
2. **Pinned attribution**
   - map the target against WiiCompiled
     `a135beb201042b20f390c6695ca6b26768820fb4`;
   - use RMCP01 / decomp / DTK attribution only as needed;
   - distinguish upstream semantics, hardware evidence, and inference.
3. **Bounded implementation**
   - implement the observed boundary, or a documented bounded audited GX setter family;
   - preserve pinned argument/register semantics;
   - for each batch member, audit both the pinned native wrapper and the real
     Aurora implementation, including conversions and state/FIFO effects;
   - require a separate diagnostic stage and synthetic dispatch/link coverage
     for every member, plus executable argument/context-preservation tests;
   - identify members not yet reached on hardware as pre-ported, not validated;
   - keep unknown boundaries as hard stops; a batch may include the exact
     guest GXData mirror required by its audited setters, with executable
     memory contracts. Other guest-memory, callback, scheduler, resource, DVD,
     input, and audio behavior remains hardware-driven.
4. **Nintendo-data-free validation**
   - add or update narrow synthetic/link coverage where practical;
   - require the five repository workflows to pass on the exact candidate
     revision (the exact PR HEAD when a PR is used):
     `lint`, `fast-track-startup`, `bootstrap-register-prelude`,
     `stateful-translated-sequence`, and `build-switch`;
   - `build-switch` must additionally syntax-compile every
     `source/*_hle_bridge.cpp` that contains a
     `MKW_LOCAL_RENDERED_FAST_TRACK` branch with that branch enabled, using
     devkitA64 plus the pinned WiiCompiled/Aurora headers. This compile gate is
     Nintendo-data-free and exists specifically to catch rendered-only C++
     errors (header collisions, ambiguous overloads, missing declarations,
     signature drift) before merge.
5. **Private rendered build gate**
   - build `scripts/build-local-rendered-fast-track.sh` successfully from the
     exact candidate revision, or build the same rendered/Discovery CMake
     target directly in the validated prepared tree, recording dependency
     pins, mode, command and candidate source hashes locally;
   - this is a required sixth gate for rendered RMCP01 work because the public
     CI graph does not include the local game-derived product or the complete
     Aurora/Dawn/NVK rendered target.
6. **Real-Switch validation**
   - run the exact locally built NRO;
   - compare the new durable diagnostics against the previous hardware baseline;
   - accept the change only if the blocker is genuinely crossed according to
     the rule below.

## Definition of "hardware-crossed"

A dispatch hit counter is useful telemetry, but it is incremented when the
target is observed. It does not, by itself, prove that the native bridge
completed successfully.

A boundary is considered **hardware-crossed** only when all applicable evidence
supports progression beyond it:

```text
target is attributable to the executed path
  (first-hit record, counter, or verified caller/control-flow evidence)
AND current blocker != that target
AND execution reaches a later durable dispatch / milestone
```

A later exact blocker is the strongest ordinary proof. An explicit clean return
to a later known state or milestone can also qualify when the control flow does
not naturally produce another blocker.

Do not mark a boundary hardware-crossed solely because `GX... hits = 1`.
Discovery records entries before invocation and only the first occurrence of
an address. For a repeated loop, a later caller can establish all returns
when the verified control flow, captured arguments, stack/fiber state and
counter agree. State that inference explicitly; do not turn one first-hit
line into a claim of individually captured calls. Acceptance remains scoped
to the executed argument family, not every branch of the bridge.

Bind each run to its candidate revision and exact NRO size/SHA-256, successful
launch/transfer record and raw-report manifest. Compare report hashes against
the previous baseline. Files retained on SD can be byte-identical or stale;
MTP timestamps may be unavailable. A successful nxlink transfer, host test,
private link, or static coverage entry does not prove a native return or an
observed image. Newly recorded `elapsed_ms` measures host time from the first
translated dispatch, excluding transfer time.

## Hardware invariants checked on every rendered run

Compare the new run with the previous accepted baseline.

### Runtime / scheduler

- previously crossed critical boundaries do not regress;
- `TaskThread::run` remains reachable when expected;
- guest fiber / OS current / OS running identities remain coherent;
- default/main recovery remains coherent when the tested path previously
  returned there;
- no new native exception appears before the intended frontier.

### Guest data / filesystem

- FST remains published and structurally valid when expected;
- DVD/resource success is never inferred from FST publication alone;
- missing `dvd-read-status` remains "not observed", not success;
- guest pointers and memory operands are interpreted only from pinned semantics
  and captured evidence.

### Graphics

Track separately:

- FIFO write count and last event/byte;
- display-list calls;
- FIFO-produced drawable work;
- `GXCopyDisp` calls;
- present successes and failures;
- renderer/frame lifecycle state.

GX-state FIFO writes alone are **not** proof that RMCP01 drawing works. Compare
the counters in the attributable durable reports for each run; snapshots may
precede a later bridge and cannot prove that bridge's FIFO effects.

The first transition to drawable FIFO work, a game-facing display list,
`GXCopyDisp`, or a successful RMCP01 present is a distinct milestone and must
be recorded as such.

## Blocker diagnostics

The current durable blocker format is sufficient for many scalar-call
boundaries, but not for every future ABI or guest-memory problem.

When a future blocker cannot be understood from the existing fields and pinned
semantics, prefer a narrow diagnostic change before guessing. Useful additional
state includes:

```text
PC / target / stage
LR / CTR / CR
r1 / r2 / r3-r10 / r13
current guest fiber / OS current / OS running
guest pointer operands and bounded local memory samples when specifically needed
```

Game-derived memory samples remain local diagnostics and must not become public
fixtures or committed Nintendo-derived data.

## Scope discipline

The default remains **hardware-first**. Discovery may additionally pre-port a
bounded family of audited GX setters in one candidate, one rendered
build, and one hardware run. See `GX_SCALAR_BATCH_2026-10-02.md` for the first
batch and its explicit evidence limits.

Preparing generic instrumentation, build checks, or reusable dispatch plumbing
is allowed. Audited batching is not permission to skip unknown calls, guess
guest-memory values, or substitute success for unimplemented stateful behavior.

If a new run:

- crosses the current blocker and exposes target X: record X and audit the next
  bounded candidate;
- hits the same blocker again: fix only that current boundary;
- raises a host/native exception first: that exception becomes the frontier;
- runs without a blocker: use liveness/invariant evidence before assuming
  success or adding more HLE.

## Current frontier — 2026-10-06

The [fresh GXCopyTex hardware result](HARDWARE_RESULTS_2026-10-06_GX_COPY_TEX_PIX_MODE_SYNC_FRONTIER.md)
accepts the observed native RGB5A3 128×128 copy to `0x9210A720`, clear 1,
using its new copy-pass report, verified caller and later DIRECT blocker
GXPixModeSync `0x8016EB70`, stage GX_COPY_TEX, 120.375 seconds. All 37 reports /
629,452 bytes are independently checked, 13 changed / 24 identical. The user
reports black then error. Native return does not establish GPU completion,
copied pixels or later retirement; the preceding present counts do not identify
images. PixModeSync must preserve the pinned guest-mirror-before-native order
and emit the real Aurora pixel-engine control command. The
[validated candidate](GX_PIX_MODE_SYNC_2026-10-06.md) supplies that ordering;
21 suites, five exact-code workflows / six jobs and the private build pass.
Transfer and native console return remain pending reconnection.

On October 5 the user authorized managing PRs and merging when their pipelines
pass, without another confirmation. Merge status and hardware acceptance are
recorded separately: merging a validated candidate does not label its pending
native return or image hardware-crossed. Private rendered-build and exact NRO
checks remain required before deployment.

## Earlier frontiers — 2026-10-03

The latest attributable real-Switch evidence is recorded in
[HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md):

- GXSetCoPlanar, GXSetClipMode, GXSetIndTexMtx, GXSetIndTexCoordScale and
  GXSetChanAmbColor are hardware-crossed;
- the exact IA8 descriptor loads on maps 0..7 remain crossed;
- GXLoadTexMtxImm remains accepted for the ten type-0 loop loads, IDs
  30,33,...57, as recorded in the dated matrix baseline;
- all eight `Gen2(c,1,4,60,0,125)`, `Scale(c,0,0,0)` and `Bias(c,0,0)` triples
  are accepted for c=0..7;
- restored caller `0x80241380`, dispatch 605620, captures r3=7, r8=125 and
  stage `RMCP01_GX_SET_TEX_COORD_BIAS`; coherent state and verified loop
  control flow support repeated returns without logging every call;
- the new DIRECT blocker is GXSetTevDirect (`0x80171B58`), stage ID 0,
  LR `0x80240F98`, stage `RMCP01_GX_SET_NUM_TEV_STAGES`, dispatch 605633;
- elapsed time is 100,205 ms after the first dispatch, and the action
  remains abort after durable blocker record; Direct has arrived, not returned;
- the preceding durable snapshot is at dispatch 605367, before the loop,
  with 99 successful presents, no present failures, valid FST and coherent
  guest scheduler identities;
- pinned WiiCompiled remains `a135beb201042b20f390c6695ca6b26768820fb4`;
- Scale-to-later-caller is +35 versus the callback-free forecast +23, and
  later-caller-to-frontier is +13 versus +1. These deltas are compatible with
  VI callback polling, without establishing an exact callback count;
- the earlier FIFO/present counters do not independently measure later
  native writes or prove recognizable pixels. The user saw a black screen;
- watchdog history records 94 ACTIVE samples and one one-second STALE interval,
  then ACTIVE at 95,808 ms and later progression, rather than a persistent stall.

The scalar batch covers GXSetClipMode, GXSetDither and GXSetDstAlpha. The
latter two remain unreached. The indirect, ambient, bounded IA8 and current
matrix and disabled coordinate candidates are hardware-accepted within their
documented scopes.
The coordinate neighbors are audited in
[GX_TEX_COORD_NEIGHBORS_2026-10-02.md](GX_TEX_COORD_NEIGHBORS_2026-10-02.md).
The [bounded coordinate candidate](GX_TEX_COORD_BATCH_2026-10-03.md), code
`91a4a01`, passed local host/workflow checks and its private Rendered Discovery
build. NRO `64ba8377...` transferred with nxlink exit 0 at 2026-10-02
23:09:38 UTC. Retrieval on October 3 produced 28 reports, 526,932 bytes, with
12 changed files and verified hashes/ZIP CRC. Enabled Scale/Bias branches
and arbitrary sizes still have host contracts only. Native return does not
establish every best-effort guest-mirror write completed; no GXData dump was
captured. The remaining TEV neighbors stay static forecasts; Direct stage 0
arrival does not accept its return or stage IDs 1..15. No unknown/stateful call
is skipped to suppress an exit.

The subsequent [audit run](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md),
`b3484117` / `7ecbc8a9...`, preserves this normal path and reaches Direct
stage 0 at dispatch 608381, elapsed 107,925 ms. It establishes normal-path
non-regression, while negative failure branches remain host/static evidence.
The user again saw black. The subsequent [six-setter TEV batch](GX_TEV_SCALAR_BATCH_2026-10-03.md)
passed local contracts, all five GitHub workflows on code `e76e8f38` and the
private build. Its [fresh hardware result](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md)
accepts the caller default tuples on stages 0..15 by six first hits, checked
loop control flow, coherent state and the later KColor frontier at 605056.
This is 96 new calls plus 16 existing Order calls, not individual return
tracing or hardware proof of alternate inputs. The user reported black output
and an error at exit. KColor ID 0 / pointer `0x80398FCC` is arrived at, not
returned; the actual RGBA bytes remain unknown.
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
recognizable game pixels remain unproven.

The separate [audit candidate](PORT_AUDIT_2026-10-03.md), code `b3484117`,
NRO `7ecbc8a9...`, passed local gates, its private build and the attributable
normal-path console run above. Its corrected failure branches retain their
separate host/static evidence and were not exercised by that console run.

## Governance note

The five public workflow checks are currently a project process rule. They do
not replace the private rendered-build gate, and branch settings should not be
assumed to enforce the full validation policy automatically.


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
