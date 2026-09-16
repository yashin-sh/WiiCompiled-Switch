# PAL OSResumeThread — 0x801AA58C

Tracking: #117

## Hardware evidence

A real Switch run from merged `main` `0cea854e009da1fe2881592d3b6066cba2843aca` crossed `OS__InitMessageQueue` and stopped at:

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

`r3=0x8042A680` is the guest `OSThread*` seen at the preceding `OSCreateThread` boundary, proving the created-suspended thread is now being resumed.

## Pinned WiiCompiled mapping

At pinned commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801AA58C` is `OSResumeThread`.

The pinned HLE decrements the thread suspend count at `+0x2CC`, and when it reaches zero repairs the thread's effective priority and queue linkage. A READY thread is inserted into the priority run queue rooted at `0x803477B0`; the pending mask at `0x80386920` and reschedule state at `0x8038691C` are updated. If rescheduling is pending, the pin immediately enters `SelectThread(0)` at `0x801A9C08` with guest interrupts still disabled.

Desktop `GuestFiberManager` resume/suspend calls and its sleep-timer/park bookkeeping are host implementation details, not guest memory semantics.

## Switch fast-track contract

The Switch bridge mirrors the guest-visible suspend count, effective-priority and queue state. It supports the hardware-proven READY path directly and preserves guest-memory-only WAITING queue repair. Paths that require unproven inherited-mutex propagation or desktop-fiber Running-state recovery remain explicit durable blockers.

The scheduler transition is not skipped. When the pinned path requests a reschedule, the bridge dispatches the exact `SelectThread(0)` target (`0x801A9C08`). If that native scheduler boundary is not yet available, it becomes the next durable blocker rather than being approximated.

## Acceptance

On the next hardware run, `0x801AA58C` must no longer be the first unsupported direct dispatch. `0x801A9C08` is an expected next blocker if the newly resumed thread causes the pinned immediate reschedule path; otherwise the next durable boundary defines the following step.
