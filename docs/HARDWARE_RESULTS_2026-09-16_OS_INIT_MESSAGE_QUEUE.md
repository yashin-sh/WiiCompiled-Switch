# Hardware result: OS message queue init crossed to OSResumeThread

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch run from `main` at `e1465f9650f54b3e228a5d8c8686ed1aa2fa7095` first crossed the PAL `OSCreateThread` native boundary added in PR #138 and stopped at `OS__InitMessageQueue` (`0x801A72FC`). PR #139 then added the pinned guest-visible message-queue initialization bridge.

The subsequent hardware run from merged `main` `0cea854e009da1fe2881592d3b6066cba2843aca` crossed `OS__InitMessageQueue` and stopped at:

```text
kind    : DIRECT
target  : 0x801AA58C
pc      : 0x800060A4
r1      : 0x803990F8
r2      : 0x8038EFA0
r3      : 0x8042A680
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

The `r3` value is the same guest `OSThread*` previously observed at `OSCreateThread`, so the new blocker is the expected resume of the thread that was created suspended.

This validates the `OS__InitMessageQueue` bridge on real hardware far enough to reach the guest scheduler/thread-resume path.

## Pinned WiiCompiled mapping

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801AA58C` is `OSResumeThread`.

The pinned native HLE:

- disables guest interrupts and reads the target thread suspend count at `+0x2CC`;
- decrements it, clamping negative results to zero;
- when the count reaches zero, restores the thread's effective priority/queue linkage;
- for a READY thread, inserts it into the RVL run queue selected by its effective priority and marks the scheduler reschedule state;
- when rescheduling is requested, enters `SelectThread(0)` at `0x801A9C08` while interrupts are still disabled;
- returns the pre-decrement suspend count in `r3` when the scheduler path returns.

Pinned desktop `GuestFiberManager` calls and sleep/park bookkeeping are host-only implementation details. They have no corresponding Horizon state and are not fabricated by the Switch bridge.

## Switch fast-track contract

The hardware-proven path is the newly-created READY thread path. The Switch bridge mirrors its guest-visible suspend-count, effective-priority, run-queue and scheduler-pending mutations. Waiting-thread queue repair is retained where it is guest-memory-only; inherited-mutex priority propagation and the desktop-only Running-state recovery remain explicit durable boundaries rather than guessed behavior.

Most importantly, the bridge does not silently skip the scheduler transition. If the pinned path requests rescheduling, it hands off to the exact `SelectThread(0)` address (`0x801A9C08`). Until that scheduler boundary itself is hardware-proven and ported, it remains the next attributable blocker.

A Nintendo-data-free synthetic probe keeps `0x801AA58C` represented in CI.

## Acceptance

After this bridge merges, rebuild the local fast-track NRO from `main` and run it on real Switch hardware. `0x801AA58C` must no longer be the first unsupported direct dispatch. If the READY-thread path requests an immediate reschedule, `0x801A9C08` is the expected next durable scheduler boundary; otherwise the next observed boundary defines the following step.
