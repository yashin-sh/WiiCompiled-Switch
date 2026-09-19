# Hardware result — RMCP01 FST, DVD-read reachability and later priority-6 thread

Date: 2026-09-19  
Tracking: #117, #154, #162  
Relevant merges: #183, #184, #185, #186

## Result

The local RMCP01 resource bootstrap is now hardware-proven through FST
publication, but the current startup path does **not** yet reach the local
DVD-read overrides.

Observed FST publication:

```text
status=published
address=0x97dc0000
size=64224
entries=2096
```

The address matches the reserved MEM2 FST region below the IPC arena. This
proves the user-owned `DATA/sys/fst.bin` is validated, copied into guest
memory and published through low memory `0x80000038` / `0x8000003C`.

## Graphics state

The rendered fast-track still reports only the eight early video-configuration
FIFO writes. They classify as BP registers `0x49`, `0x4A`, `0x4D` and
`0x4E`. There is still no display-list work, drawable FIFO work,
`GXCopyDisp` or successful RMCP01 present.

The renderer itself remains hardware-proven independently; this run does not
identify a renderer failure.

## DVD-read reachability

PR #185 adds the narrow local `DVDReadPrio` / internal
`DVDReadAsyncPrio` bridge against the user's extracted `DATA/files`.

The first hardware run after #185 does not reach either boundary:

- `DVDReadPrio`: `0x8015E834`
- `DVDReadAsyncPrio`: `0x8015E74C`

No `dvd-read-status.txt` is produced. Therefore the local DVD-read
implementation remains available for the first real read, but it is not the
current startup blocker.

## Scheduler attribution

The initial priority-6 `EGG::ProcessMeter` OSThread is created at
`0x8042A680`. Hardware shows that this thread reaches its expected
`OSReceiveMessage` / `OSSleepThread` blocking path and the default thread
continues afterwards.

Only later, between roughly 2 and 3 seconds into the run, execution switches
durably to a different OSThread:

```text
active OSThread : 0x90112660
priority        : 6
state           : RUNNING

default OSThread: 0x80347498
priority        : 16
state           : READY
```

The sustained translated sample remains:

```text
target   : 0x8020FCD4  PostRetraceCallback
guest pc : 0x8024373C  EGG::Thread::start(void*)
```

This disproves the earlier attribution of the durable priority-6 loop to the
initial ProcessMeter thread.

## Diagnostic added by #186

PR #186 adds bounded, behavior-neutral guest-thread lifecycle telemetry:

```text
/switch/WiiCompiled-Switch/fast-track-thread-events.txt
```

It records `OSCreateThread -> OSResumeThread -> GuestFiberEntry` state,
including:

- OSThread pointer;
- translated entry point and entry argument;
- requested/effective priority, state, suspend count and queue;
- current guest fiber and OS current/running context;
- EGG object vtable;
- virtual destructor / `run()` / `onExit()` / `onEnter()` slots.

The next hardware gate is to identify the owner/class and virtual `run()`
implementation for OSThread `0x90112660`. Scheduler priorities, DVD behavior
and renderer behavior must remain unchanged until that identity is known.

## #186 result and next exact boundary

The next hardware run produced the lifecycle event that #186 was designed to
capture:

```text
thread=0x90112660
entry=0x8024373c
arg=0x8042e930
requested_prio=6
vtable=0x80270bc0
vt_dtor=0x80008cb0
vt_run=0x80008d18
```

The same run finishes with the default thread READY at priority 16 and
`0x90112660` RUNNING at priority 6. It records:

```text
VIWaitForRetrace hits : 928
OSWakeupThread hits   : 925
SelectThread hits     : 6
```

The old Switch `VIWaitForRetrace` implementation used the pin's non-fiber
fallback even after HostContext guest fibers became available. Sleeping the
Horizon host thread paced time but did not transition the guest OSThread to
WAITING, so the priority-6 guest continuously starved the default thread.

The next implementation gate is therefore exact and pinned: use
`OSSleepThread` on VI queue `0x80386BC0` for active guest fibers and service
due retraces synchronously from safe runtime call boundaries. The first
hardware acceptance condition is scheduler progression, not a rendered frame.
