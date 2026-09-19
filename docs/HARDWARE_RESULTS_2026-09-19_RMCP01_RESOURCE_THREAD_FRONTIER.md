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
