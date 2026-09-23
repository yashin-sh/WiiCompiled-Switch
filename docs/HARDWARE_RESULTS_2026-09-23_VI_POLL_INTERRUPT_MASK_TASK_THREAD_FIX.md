# Hardware result — VI poll re-entry clobbers TaskThread receive slot (2026-09-23)

Tracking: #117, #154, #162

## Result

The rendered real-Switch run preserves the hardware-proven graphics path and
pinpoints the TaskThread corruption to the call boundary used for
`OSWakeupThread`.

The receive-phase trace records:

```text
ready-before-dequeue:
  out_value = 0x8042E448
  first = 0
  used = 1

after-dequeue:
  msg = 0x8042E7DC
  out_value = 0x8042E448
  first = 1
  used = 0

after-output-write:
  msg = 0x8042E7DC
  out_value = 0x8042E7DC

after-wakeup-senders:
  msg = 0x8042E7DC
  out_value = 0x8042E448

after-restore-interrupts:
  out_value = 0x8042E448
```

The message queue producer is already proven correct:
`TaskThread::request` sends `mJobs[0]=0x8042E7DC` into queue
`0x8042BBFC` / array `0x8042E7A8`.

Therefore the queue/dequeue/output write are correct. The output slot is
clobbered specifically across the `InvokeDirectCpu<0x801AAAA4>` call used to
wake blocked senders.

## Empty sender wait queue

At both the valid output-write sample and the corrupted post-wakeup sample:

```text
send_head = 0
send_tail = 0
```

So this call has no sender thread to wake. The native OSWakeupThread bridge
therefore does not enter its runnable-thread/reschedule path for this queue.

## Switch-specific pre-call VI poll

The Switch `InvokeDirectCpu` path executes `ApplyRuntimeCallOptions` before
both native and translated calls. In rendered/local execution that helper calls
`mkw_switch_hle_vi_poll_retrace(cpu)`.

At this point `OSReceiveMessage` is inside its critical section: it disabled
guest interrupts before inspecting the queue and has not restored them yet.

The Switch poll can therefore inject a due VI retrace/callback at a point where
the guest has interrupts disabled. Callback/translated stack frames can use the
same guest `r1` area as the caller-owned receive scratch slot. The observed
stack-shaped overwrite is:

```text
r1             = 0x8042E458
r1 - 0x10      = 0x8042E448
r1 - 0x20      = 0x8042E438   <- receive output slot
```

and after the call the output slot contains `0x8042E448`.

## Pinned WiiCompiled behavior

Pinned WiiCompiled exposes `OS_HLE_InterruptsEnabled()`. Its ordinary VI
polling is serviced from scheduler execution points after interrupts are
enabled, while its deferred retrace callback path explicitly returns without
delivery when interrupts are disabled.

The Switch port's unconditional translated-call-boundary polling is therefore
too permissive.

## Minimal correction

Expose the existing Switch guest interrupt-enabled state and make
`mkw_switch_hle_vi_poll_retrace` return immediately while guest interrupts are
disabled.

No TaskThread, OSMessageQueue, OSWakeupThread, scheduler, HostContext, GX, DVD,
or renderer semantics are changed.

## Hardware acceptance

After public CI and the private rendered build:

1. preserve real FIFO work, GXCopyDisp/GXFlush and successful presents;
2. verify `after-output-write` stays `0x8042E7DC`;
3. verify `after-wakeup-senders` also stays `0x8042E7DC`;
4. verify the TaskThread callback target is no longer the stack pointer
   `0x8042E458`;
5. follow the next exact durable blocker if execution advances beyond this
   TaskThread job.

## Hardware validation

The next rendered run validates this correction. The TaskThread receive slot
contains `0x8042E7DC` after the output write, after the sender-wakeup
boundary, and after interrupt restore. The old stack-shaped
`0x8042E448 -> callback 0x8042E458` path is gone.

The real job dispatch is:

```text
job      = 0x8042E7DC
callback = 0x8000B53C
arg      = 0
onDone   = 0
```

Execution progresses through a successful local DVD read of
`/Boot/Strap/eu/English.szs` before reaching the distinct
`SELECTTHREAD_IDLE_POLL` scheduler frontier.

See
`HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md`.
