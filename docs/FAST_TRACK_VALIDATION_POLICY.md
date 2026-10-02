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

## Current frontier

The latest attributable real-Switch evidence is recorded in
`HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_TEX_COORD_SCALE_FRONTIER.md`:

- GXSetCoPlanar, GXSetClipMode, GXSetIndTexMtx, GXSetIndTexCoordScale and
  GXSetChanAmbColor are hardware-crossed;
- the exact IA8 descriptor loads on maps 0..7 remain crossed;
- GXLoadTexMtxImm is hardware-accepted for the ten observed type-0 loop
  loads, ids 30,33,...57, followed by return from Gen2 coord 0;
- the new DIRECT blocker is GXSetTexCoordScaleManually (`0x80171180`),
  with r3..r6=0, LR `0x80241334`, stage `RMCP01_GX_SET_TEX_COORD_GEN2`;
- its newly instrumented elapsed time is 99,513 ms after the first dispatch,
  and the action remains abort after durable blocker record;
- the preceding durable snapshot is at dispatch 605265, before matrix setup,
  with 99 successful presents, no present failures, valid FST and coherent
  guest scheduler identities;
- pinned WiiCompiled remains `a135beb201042b20f390c6695ca6b26768820fb4`;
- matrix returns are proven by executed control flow, a distinct later
  frontier and +10 dispatch delta, not ten individually captured log hits;
- the earlier FIFO/present counters do not independently measure the new
  matrix writes or prove recognizable pixels. This launch's screen
  observation is pending; the preceding run showed black.

The scalar batch covers GXSetClipMode, GXSetDither and GXSetDstAlpha. The
latter two remain unreached. The indirect, ambient, bounded IA8 and current
matrix candidates are hardware-accepted within their documented scopes.
The coordinate neighbors are audited in
[GX_TEX_COORD_NEIGHBORS_2026-10-02.md](GX_TEX_COORD_NEIGHBORS_2026-10-02.md).
The [bounded coordinate candidate](GX_TEX_COORD_BATCH_2026-10-03.md), code
`91a4a01`, passed local host/workflow checks and its private Rendered Discovery
build. NRO `64ba8377...` transferred with nxlink exit 0 at 2026-10-02
23:09:38 UTC. Fresh reports and a screen observation for it are pending:
Scale is observed at the prior blocker; Bias and Gen2 coords 1..7 have no
hardware acceptance yet. Enabled Scale/Bias branches have host contracts
only. The next TEV caller/frontier is a static forecast, not a runtime result. Native
forwarding alone would omit Scale/Bias guest state mirrors. Adjacent TEV and
pixel-state calls remain forecasts until reached or explicitly audited for
a bounded candidate. No unknown/stateful call is skipped to suppress an exit.

## Governance note

The five public workflow checks are currently a project process rule. They do
not replace the private rendered-build gate, and branch settings should not be
assumed to enforce the full validation policy automatically.
