# Hardware result: OSReceiveMessage crossed into OSSleepThread

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch fast-track run from merged `main` after PR #144 stopped at:

```text
WiiCompiled-Switch unsupported translated dispatch
kind                  : DIRECT
target                : 0x801aa9b8
guest pc              : 0x8024373c
r1                    : 0x8042a628
r2                    : 0x8038efa0
r3                    : 0x804294f8
r13                   : 0x8038cc00
fast-track stage      : GUEST_POST_MAIN_ACTIVE
action                : abort after durable blocker record
```

This validates the PAL `OSReceiveMessage (0x801A7424)` bridge on real hardware far enough to enter its empty blocking-receive path. The previous queue base was `0x804294F0`; the new `r3=0x804294F8` is exactly the embedded receive `OSThreadQueue` at `queue + 0x08`.

The guest PC remains `EGG::Thread::start (0x8024373C)`, so the restored EGG thread is now waiting for a message rather than failing inside the context-load path.

## Pinned WiiCompiled mapping

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801AA9B8` is `OSSleepThread_HLE_801aa9b8`.

Its guest-visible behavior is:

- reject a null wait-queue pointer;
- disable interrupts for the operation;
- obtain the running guest `OSThread` from `0x800000E4`, using the pinned default-thread fallback only when no running thread is published;
- refuse to park while the OSDisableScheduler nesting count at `0x80386918` is non-zero;
- mark the current thread `WAITING` (`state=4`);
- publish the wait-queue pointer at thread offset `0x2DC`;
- insert the thread into the `OSThreadQueue` in effective-priority order using priority offset `0x2D0` and next/prev offsets `0x2E0/0x2E4`;
- set the reschedule counter at `0x8038691C`;
- yield through `SelectThread(0)`;
- if the scheduler returns without actually switching, unlink the thread again and restore its `RUNNING` state rather than leaving a duplicate/stale wait-queue node;
- restore the prior interrupt state on a normal return.

The desktop `GuestFiberManager` suspension is host machinery rather than guest-visible state. The Switch port does not fabricate it: the already hardware-proven `SelectThread -> OSLoadContext` path remains responsible for switching guest contexts.

## Switch fast-track contract

The Switch bridge implements only the proven `OSSleepThread` boundary and its wait-queue/scheduler handoff. It does not pre-port `OSWakeupThread (0x801AAAA4)` or any unrelated timer/alarm/fiber behavior.

A Nintendo-data-free synthetic probe asserts native registration, while the synthetic fast-track keeps the real helper reachable through `--gc-sections` to catch link regressions.

## Acceptance

Rebuild from merged `main` and run on real Switch hardware. `0x801AA9B8` must no longer be the durable unsupported `DIRECT` target. The next durable result should show the scheduler handoff, a wakeup-related boundary, or another later post-main blocker.