# PAL OSReceiveMessage — 0x801A7424

Tracking: #117
Date: 2026-09-16
Pinned WiiCompiled: `a135beb201042b20f390c6695ca6b26768820fb4`

## Hardware blocker

```text
kind    : DIRECT
target  : 0x801A7424
pc      : 0x8024373C
r1      : 0x8042A628
r2      : 0x8038EFA0
r3      : 0x804294F0
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This is the first unsupported native boundary after the hardware-proven `OSLoadContext` bridge restored and jumped into `EGG::Thread::start` at `0x8024373C`.

## Pinned semantics

Pinned WiiCompiled native-overrides `OSReceiveMessage`. It disables interrupts, checks the `OSMessageQueue` ring-buffer state, and either dequeues one message or follows the blocking/non-blocking empty-queue behavior.

A successful dequeue advances `first`, decrements `used`, optionally writes the message to the caller's output pointer, wakes the embedded send wait queue through `OSWakeupThread (0x801AAAA4)`, restores the prior interrupt state, and returns `1`.

An empty non-blocking receive restores interrupts and returns `0`. An empty blocking receive parks the current thread on the embedded receive wait queue through `OSSleepThread (0x801AA9B8)` and retries after wakeup.

## Switch implementation boundary

The Switch bridge mirrors the message receive/ring-buffer and interrupt-state behavior but intentionally does not pre-port `OSSleepThread` or `OSWakeupThread`. Both remain exact blocker-driven native boundaries. The next hardware result will prove which dependency this restored EGG thread requires first.

Synthetic CI retains the real helper at link time without fabricating a guest queue or scheduler state.
