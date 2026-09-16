# Hardware result: OSResumeThread crossed to SelectThread

Date: 2026-09-16
Tracking: #117

## Hardware evidence

A real Switch run from `main` at `f41339317fb32f5b4064b8402f080b194f8e7bfb` crossed the PAL `OSResumeThread` bridge added in PR #140 and stopped at:

```text
kind    : DIRECT
target  : 0x801A9C08
pc      : 0x800060A4
r1      : 0x803990F8
r2      : 0x8038EFA0
r3      : 0x00000000
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This hardware result validates the `OSResumeThread` guest run-queue update far enough to prove its pinned scheduler handoff: `SelectThread(0)` is now the first unsupported native boundary.

## Pinned WiiCompiled mapping

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801A9C08` is `SelectThread`.

The pinned native HLE owns the guest-visible RVL scheduler transition:

- return immediately while the scheduler disable/idle nesting count is non-zero;
- require the guest current context and running context to agree;
- compare the currently running thread priority with the highest pending run-queue priority for `SelectThread(0)`;
- requeue the running thread when preemption is required;
- preserve the `OSSaveContext` dependency before leaving a running context;
- select the highest-priority runnable `OSThread` from the 32 RVL priority queues;
- clear stale pending bits and terminated queue entries;
- mark the selected thread `RUNNING`, publish `OSRunningContext`, and call `OSSetCurrentContext`;
- on the non-fiber path, restore the selected guest context through `OSLoadContext`.

Desktop WiiCompiled additionally uses `GuestFiberManager` for host context switching and pumps host sleep/audio/alarm/VI events in the no-runnable-thread idle loop. Those host facilities do not exist on Horizon and must not be fabricated.

## Switch fast-track contract

The Switch bridge mirrors the guest scheduler state and the pinned no-switch/queue-selection paths without creating desktop fibers:

- scheduler-disabled, uninitialized and current/running-context mismatch paths return `0` like the pin;
- the `forceSwitch=0` priority comparison is preserved;
- run-queue enqueue/dequeue, pending-mask and reschedule state are maintained in guest memory;
- registered guest switch callbacks are attempted through the existing dynamic dispatcher and missing callbacks remain skippable like the pin;
- `OSSetCurrentContext` uses the already-validated Switch native bridge;
- an actual non-fiber context switch keeps `OSLoadContext (0x801A1F58)` as an explicit next boundary rather than implementing it speculatively;
- reaching the host-driven idle polling path is reported explicitly as `SELECTTHREAD_IDLE_POLL` until hardware proves that timer/audio/alarm/VI polling is required.

The bridge also keeps the pinned `OSSaveContext (0x801A1ED8)` dependency explicit through the translated indirect dispatcher when the current context is preemptible.

A Nintendo-data-free synthetic probe keeps direct target `0x801A9C08` represented in CI.

## Acceptance

After this bridge merges, rebuild the local fast-track NRO from `main` and run it on real Switch hardware. `0x801A9C08` must no longer be the durable unsupported `DIRECT` dispatch. The next hardware result determines whether the current thread can continue without a switch, whether `OSSaveContext` is required, or whether the scheduler selects a new thread and reaches `OSLoadContext`.
