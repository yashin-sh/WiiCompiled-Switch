# Hardware result — AsyncDisplay idle VI wake attribution (2026-09-23)

Tracking: #117, #154, #162

## Result

The follow-up rendered hardware run reproduces the post-DVD
`SELECTTHREAD_IDLE_POLL` boundary and the new scheduler diagnostics identify
the exact wait owner.

The TaskThread path remains corrected:

```text
job      = 0x8042E7DC
callback = 0x8000B53C
arg      = 0
onDone   = 0
```

The receive slot remains `0x8042E7DC` through output write, sender wakeup and
interrupt restore. The callback again reaches the local DVD bridge and reads
`/Boot/Strap/eu/English.szs` successfully.

## Default-thread sleep owner

`fast-track-os-sleep-events.txt` records:

```text
thread   = 0x80347498
queue    = 0x804294A4
state    = RUNNING before park
priority = 16
pc       = 0x800060A4
r1       = 0x80399088
lr       = 0x8020FE50
```

The same run records the `EGG::AsyncDisplay` object at `0x8042944C`
during its construction path. RMCP01's `AsyncDisplay` layout places its
thread/sync queue at offset `+0x58`:

```text
0x8042944C + 0x58 = 0x804294A4
```

So the wait queue is exactly the active `AsyncDisplay` synchronization queue.

The EGG reference implementation used by the RMCP01 header source defines:

```cpp
AsyncDisplay::syncTick():
    OSSleepThread(&mSyncQueue)

AsyncDisplay::postVRetrace():
    ...
    mTickCount++
    OSWakeupThread(&mSyncQueue)
```

This makes the required wake source unambiguous: the main/default thread is
waiting for the next VI post-retrace callback, not for a generic sleep timer,
audio event or alarm.

## Idle frontier state

At the blocker:

```text
current_context  = 0x8042E480
running_context  = 0x8042E480
guest_fiber      = 0x8042E480
scheduler_pending= 0
running_state    = WAITING
running_priority = 24
running_queue    = 0x8042BC04

default_thread   = 0x80347498
default_state    = WAITING
default_priority = 16
default_queue    = 0x804294A4
default_queue_head/tail = 0x80347498
```

There is no runnable guest thread left. The Switch port normally advances VI
from translated call boundaries, but no further translated call can occur while
every guest thread is waiting. Pinned WiiCompiled solves this exact condition
inside `SelectThread`'s host idle loop by polling VI and waiting until the next
retrace deadline.

## Minimal correction

Port only the hardware-proven VI slice of pinned `SelectThread` idle behavior:

1. publish no running guest context and use the pinned idle OSContext;
2. enable guest interrupts while idle;
3. poll already-due VI retraces;
4. host-sleep toward the next retrace deadline when none is due;
5. let translated `PostRetraceCallback` wake the `AsyncDisplay` queue;
6. suppress nested `SelectThread` recursion from `OSWakeupThread` while
   `AdvanceRetrace` is active, matching pinned
   `VI_HLE_IsAdvancingRetrace()`;
7. once the pending mask becomes non-zero, disable interrupts, clear the idle
   context and continue normal thread selection.

No sleep-timer, alarm, audio or unrelated scheduler subsystem is added.

## Validation telemetry

The candidate adds:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-select-thread-idle-recovery.txt
```

A hardware PASS requires:

- the old `SELECTTHREAD_IDLE_POLL` abort to disappear;
- `idle-recovery` to show a non-zero pending mask after VI service;
- default thread `0x80347498` to become READY/runnable and leave
  `0x804294A4`;
- durable translated execution beyond the current blocker;
- the existing TaskThread job/DVD invariants to remain valid.

The next distinct blocker, not a hit counter alone, becomes the new frontier.
