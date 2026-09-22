# Hardware result — valid TaskThread send / OSReceiveMessage stack-slot frontier (2026-09-22)

Tracking: #117, #154, #162

## Result

The rendered real-Switch run preserves the hardware-proven graphics path and
resolves the send side of the priority-24 `EGG::TaskThread` anomaly.

The new `fast-track-os-message-events.txt` records exactly one relevant send:

```text
queue       = 0x8042BBFC
msg         = 0x8042E7DC
array       = 0x8042E7A8
count       = 5
first       = 0
used_before = 0
lr          = 0x80242C6C
```

PAL `0x80242C18..0x80242C97` is
`EGG::TaskThread::request(TFunction, void*, void*)`, so the send is produced
by the expected TaskThread request path.

The later TaskThread dispatch snapshot records:

```text
queue ptr          = 0x8042BBFC
queue array        = 0x8042E7A8
queue count        = 5
queue first        = 1
queue used         = 0
member mesg buffer = 0x8042E7A8
member mesg count  = 5
jobs               = 0x8042E7DC
job count          = 5
stack memory       = 0x8042BC60
stack size         = 0x2800

out message pointer = 0x8042E438
job                 = 0x8042E448
callback            = 0x8042E458
onDone              = 0x801AA0F0
```

The message producer therefore sends the correct first `mJobs[]` slot,
`0x8042E7DC`, into the correct queue and message array. The queue also
advances from `first=0, used=1` to `first=1, used=0`, which is consistent
with one dequeue.

Despite that, the TaskThread bridge later reads `0x8042E448` from its
`r1-0x20` output slot. This value aliases the worker stack rather than
`mJobs[]`.

## Stack-shaped corruption

The observed values form a PPC stack-shaped chain:

```text
r1             = 0x8042E458
r1 - 0x10      = 0x8042E448   <- value later read as job
r1 - 0x20      = 0x8042E438   <- OSReceiveMessage output slot
[job + 0x00]   = 0x8042E458   <- decoded as callback
[job + 0x14]   = 0x801AA0F0   <- decoded as onDone / OSExitThread
```

This is consistent with guest stack-frame/back-chain data overwriting the
scratch output slot, but the exact clobbering call is not yet hardware-proven.
No behavioral stack or scheduler change is made from this pattern alone.

## Worker attribution correction

The runtime object is a priority-24 `EGG::TaskThread`, but its concrete
metadata is:

```text
mJobCount  = 5
mStackSize = 0x2800
```

That does not match the currently decompiled `System::ResourceManager`
creation shape (`14` jobs, priority `24`, stack `0xC800`). Previous
documentation that called this exact object the ResourceManager worker is
therefore superseded. Until a concrete owner is proven, documentation should
refer to it only as the priority-24 TaskThread worker.

## Preserved invariants

The same hardware run records:

```text
TaskThread::run hits  : 1
GXFlush hits          : 23
GXCopyDisp calls      : 23
present successes     : 23
present failures      : 0
FIFO produced work    : YES
```

The independent watchdog remains ACTIVE through dispatch 4203 and the FST
remains structurally valid at `0x97DC0000`.

## Diagnostic candidate

Add behavior-neutral receive-phase telemetry:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-os-receive-message-frontier.txt
```

For each blocking receive it records the output slot and queue state at:

1. ready-before-dequeue;
2. after-dequeue;
3. after-output-write;
4. after-wakeup-senders;
5. after-restore-interrupts;
6. before/after a blocking sleep when needed.

It also records the send/receive wait-queue heads/tails, current/running
OSThread, and live CPU registers.

No OSMessage, scheduler, fiber, TaskThread, DVD, GX or renderer semantics are
changed.

## Next hardware acceptance

The next real-Switch run must determine the first phase where
`outMsgPtr=0x8042E438` changes away from the valid message
`0x8042E7DC`.

- If `after-output-write` is already wrong, inspect the exact guest-memory
  write boundary.
- If it is correct after the write but wrong after `OSWakeupThread`, the
  wake/scheduler path becomes the frontier.
- If it remains correct through `after-restore-interrupts` but TaskThread
  later reads the stack value, the return/context bridge becomes the frontier.

No behavioral correction is justified before that phase is hardware-proven.
