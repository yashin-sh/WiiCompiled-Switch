# Fast-track validation policy

This document defines the validation contract for blocker-driven RMCP01 bring-up
on real Nintendo Switch hardware.

The 2026-09-20 audit confirmed that the overall strategy is sound: advance from
the first exact hardware blocker, map it against the pinned WiiCompiled
revision, implement only that boundary, validate, retest on hardware, and let
the next observed blocker decide the next change.

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
3. **Minimal implementation**
   - implement only the observed boundary;
   - preserve pinned argument/register semantics;
   - do not pre-port neighboring GX, DVD, resource, input, or audio calls.
4. **Nintendo-data-free validation**
   - add or update narrow synthetic/link coverage where practical;
   - require the five repository workflows to pass on the exact PR HEAD:
     `lint`, `fast-track-startup`, `bootstrap-register-prelude`,
     `stateful-translated-sequence`, and `build-switch`.
5. **Private rendered build gate**
   - build `scripts/build-local-rendered-fast-track.sh` successfully from the
     exact candidate revision;
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
target hit count > 0
AND current blocker != that target
AND execution reaches a later durable dispatch / milestone
```

A later exact blocker is the strongest ordinary proof. An explicit clean return
to a later known state or milestone can also qualify when the control flow does
not naturally produce another blocker.

Do not mark a boundary hardware-crossed solely because `GX... hits = 1`.

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

The latest accepted run reaches eleven FIFO writes. The two new writes are
additional GX-state traffic; with no drawable work, display list, `GXCopyDisp`,
or present they are **not** proof that RMCP01 drawing works.

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

The default remains **hardware-first, boundary-minimal**.

Preparing generic instrumentation, build checks, or reusable dispatch plumbing
is allowed when it does not implement speculative game behavior. Implementing
the next GX/resource/DVD boundary before hardware reaches it is not.

If a new run:

- crosses the current blocker and exposes target X: branch for X;
- hits the same blocker again: fix only that current boundary;
- raises a host/native exception first: that exception becomes the frontier;
- runs without a blocker: use liveness/invariant evidence before assuming
  success or adding more HLE.

## Current frontier

The latest real-Switch rendered evidence is the 2026-09-21 run after merged
#211:

- `GXSetBlendMode (0x8017277C)` is hardware-crossed;
- the new exact DIRECT blocker is PAL `GXSetColorUpdate (0x801727CC)`,
  captured with `r3 = 1`;
- its stage is `RMCP01_GX_SET_BLEND_MODE`, proving the previous bridge was
  entered and execution advanced to a later target;
- pinned WiiCompiled remains
  `a135beb201042b20f390c6695ca6b26768820fb4`;
- pinned semantics cast `r3` directly to `GXBool` and call Aurora
  `GXSetColorUpdate`;
- FIFO traffic remains at eleven writes and there is still no proven display
  list, drawable work, `GXCopyDisp`, or present.

Current `main` is now ahead of that hardware evidence:

- PR #212 merged the exact `GXSetColorUpdate` bridge as
  `8aea70d0a3a8378311428eb5420760e4f763a40d`;
- all five public workflows passed on the exact PR HEAD before merge;
- `GXSetColorUpdate` is **implemented but not yet hardware-crossed**;
- the private rendered build of the exact merged revision remains the required
  sixth gate before the next Switch run;
- the following GX/resource boundary must not be implemented until hardware
  progresses beyond `0x801727CC`.

## Governance note

The five public workflow checks are currently a project process rule. They do
not replace the private rendered-build gate, and branch settings should not be
assumed to enforce the full validation policy automatically.
