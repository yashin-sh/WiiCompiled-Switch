# PAL OSSleepThread — 0x801AA9B8

Tracking: #117
Date: 2026-09-16
Pinned WiiCompiled: `a135beb201042b20f390c6695ca6b26768820fb4`

## Hardware blocker

```text
kind    : DIRECT
target  : 0x801AA9B8
pc      : 0x8024373C
r1      : 0x8042A628
r2      : 0x8038EFA0
r3      : 0x804294F8
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This is the first unsupported native boundary after the hardware-proven `OSReceiveMessage` bridge entered its empty blocking receive path. `r3=0x804294F8` is the message queue's embedded receive wait queue (`0x804294F0 + 0x08`).

## Pinned semantics

Pinned WiiCompiled disables interrupts, resolves the running guest thread, refuses to park while scheduler-disable nesting is active, then marks the thread WAITING and links it into the supplied `OSThreadQueue` in priority order.

It sets the scheduler reschedule counter and yields through `SelectThread(0)`. If the scheduler returns without switching, it reverses the wait-queue insertion and restores the thread to RUNNING so the caller can retry safely.

The pin optionally suspends/resumes a desktop `GuestFiberManager` fiber around this guest state transition. That host-fiber machinery is not guest-visible and is not copied into the Switch bridge; the existing Switch `SelectThread -> OSLoadContext` path owns the actual guest context handoff.

## Switch implementation boundary

Implement only the guest wait-queue state and `SelectThread(0)` handoff required by this blocker. Do not pre-port `OSWakeupThread`, sleep timers, alarms, or unrelated scheduler/fiber behavior before hardware reaches them.

Synthetic CI retains the real `OSSleepThread` helper at link time and statically asserts `KnownNativeCpuCall<0x801AA9B8>`.