# Hardware result — OSDetachThread crossed / OSCancelThread frontier (2026-09-25)

Tracking: #117, #155, #162

## Durable crossing

The rendered real-Switch run built from merged PR #244 durably progresses
beyond `OSDetachThread (0x801AA4EC)` to a distinct later blocker.

The strongest durable snapshot records:

```text
dispatch count        : 24430
post-main dispatch    : 23824
RKSystem::run hits    : 1
StaticR dispatches    : 269
TaskThread::run hits  : 2
AsyncDisplay endRender: 84
GXFlush hits          : 84
RMCP01 FIFO writes    : 1240
FIFO produced work    : YES
GXCopyDisp calls      : 84
present successes     : 84
present failures      : 0
```

The independent liveness watchdog remains ACTIVE through the final samples.
Resource and renderer invariants remain healthy.

## New exact blocker

```text
kind   = DIRECT
target = 0x801AA1D4
r3     = 0x901187C0
r4     = 0x00000005
r5     = 0x00000000
r6     = 0x9011375C
r7     = 0x90113774
r8     = 0x90113774
stage  = RMCP01_GX_FLUSH
```

Pinned WiiCompiled maps `0x801AA1D4` exactly to `OSCancelThread`.

The live `r3=0x901187C0` is the same TaskThread OSThread whose first
OSDetachThread path was just hardware-crossed.

## Pinned OSCancelThread semantics

Pinned WiiCompiled:

1. disables interrupts;
2. reads thread state and returns early for state 3, 0, or >4;
3. READY: removes from run queue if not suspended;
4. RUNNING: marks scheduler reschedule;
5. WAITING: removes the thread from its wait queue;
6. executes TerminateThreadCommon:
   - clears guest OSContext;
   - if detached, removes the thread from the global list;
   - sets final guest state to 0;
   - marks/destroys the corresponding host fiber;
   - unlocks all mutexes owned by the thread;
   - wakes joiners;
7. selects another thread if scheduler reschedule is pending;
8. restores interrupts.

The Switch guest-fiber seam currently exposes create/suspend/resume/switch but
no exact equivalent of pinned `ExitGuestThread`. Therefore implementing the
cancel before observing the exact queue/mutex/scheduler state would risk
leaving a stale HostContext or fabricating scheduler progress.

## Diagnostic-only candidate

For exactly `OSCancelThread (0x801AA1D4)`, the durable blocker now captures:

- state / attributes;
- suspend count / priority;
- wait queue / next / prev;
- wait-queue head / tail and whether it is a scheduler run queue;
- join queue head / tail;
- current mutex and owned-mutex list head / tail;
- global thread-list next / prev and head / tail;
- scheduler reschedule and pending masks;
- guest-fiber known/current state;
- OS current/running contexts.

No scheduler state is modified. No OSCancelThread HLE is installed.

## Next hardware acceptance

Run the private rendered build and inspect `fast-track-dispatch-blocker.txt`
first. If the target remains `0x801AA1D4`, the captured state will define the
only cancel branch eligible for implementation.

No neighboring OS thread API is pre-ported.
