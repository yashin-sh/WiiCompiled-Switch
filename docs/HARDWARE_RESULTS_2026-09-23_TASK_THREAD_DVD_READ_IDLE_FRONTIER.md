# Hardware result — TaskThread callback / first DVD read / SelectThread idle frontier (2026-09-23)

Tracking: #117, #154, #162

## Result

The first rendered hardware run after the VI interrupt-mask correction validates
the previous TaskThread fix and reaches a new scheduler boundary.

### VI interrupt-mask correction is hardware-proven

The receive-phase trace now records the valid TaskThread job pointer through the
entire receive completion path:

```text
after-output-write:
  msg       = 0x8042E7DC
  out_value = 0x8042E7DC

after-wakeup-senders:
  msg       = 0x8042E7DC
  out_value = 0x8042E7DC

after-restore-interrupts:
  msg       = 0x8042E7DC
  out_value = 0x8042E7DC
```

The previous stack-shaped `0x8042E448` overwrite no longer occurs. The
TaskThread dispatch therefore uses the real job record:

```text
job      = 0x8042E7DC
callback = 0x8000B53C
arg      = 0
onDone   = 0
```

The old `INDIRECT_CALL_MISS 0x8042E458` stack-pointer callback is gone.

## First local RMCP01 DVD read on hardware

The callback executes far enough to drive the local DVD bridge and produces:

```text
status    = read-pass
fileInfo  = 0x8042E38C
buffer    = 0x94226C20
length    = 300000
offset    = 0
result    = 299969
entry     = 10
size      = 299969
path      = /Boot/Strap/eu/English.szs
```

This is the first hardware proof that the user-owned RMCP01 FST/file mapping is
not only published but can service a real game resource read through the boot
path.

## New exact blocker

After the callback returns, the priority-24 TaskThread loops and performs its
next blocking `OSReceiveMessage`. The new durable blocker is:

```text
kind   : SELECTTHREAD_IDLE_POLL
target : 0x801A9C08
pc     : 0x8024373C
r1     : 0x8042E458
r3     : 0
r4     : 0x8042E438
r5     : 1
stage  : HOST_CONTEXT_SWITCH_ENTER
```

The durable snapshot around this point records:

```text
OSReceiveMessage hits : 4
TaskThread::run hits  : 1
guest fiber current   : 0x8042E480
OS current/running    : 0x8042E480 / 0x8042E480

default thread state  : WAITING
default thread queue  : 0x804294A4
active priority       : 24
scheduler pending     : no runnable target at the blocker
```

Pinned WiiCompiled's `SelectThread` enters a host-driven idle loop when there
is no runnable thread. That loop can service multiple independent wake sources:
sleep timers, audio DMA timing, VI retraces, host fiber timer events and OS
alarms.

The current hardware evidence does not yet identify which of those sources is
responsible for waking the default thread from queue `0x804294A4`. Porting
the complete desktop idle loop would therefore violate the blocker-driven
policy.

## Diagnostic candidate

Add behavior-neutral rendered telemetry only:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-os-sleep-events.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-select-thread-idle-frontier.txt
```

`fast-track-os-sleep-events.txt` records each actual `OSSleepThread` park:

- current thread;
- wait queue;
- state and priority;
- queue head/tail;
- current/running context;
- pc/r1/r3/r4/r5/lr.

`fast-track-select-thread-idle-frontier.txt` records the exact state when
`SELECTTHREAD_IDLE_POLL` is reached:

- current/running context;
- current guest fiber;
- scheduler idle/reschedule/pending words;
- running thread state/priority/wait queue;
- default thread state/priority/wait queue;
- queue head/tail;
- live CPU registers.

No scheduler wake source, timer, alarm, audio, VI, TaskThread, DVD or graphics
behavior is changed.

## Graphics status

This run aborts at the new scheduler frontier before the previously
hardware-proven full drawable/present sequence. It initializes the renderer and
emits the eight bootstrap GX FIFO writes, but does not reach real drawable work
or `GXCopyDisp` before the deliberate idle blocker.

That does not invalidate the earlier hardware proof of real FIFO work,
23 `GXCopyDisp` calls, 23 successful presents and `GXFlush`; this run simply
terminates earlier on the newly exposed resource/scheduler path.

## Next hardware acceptance

The next run should first identify the exact caller that parked the default
thread on `0x804294A4` and the exact idle-state ownership at the blocker.

Only after that evidence should one of the pinned idle wake sources be ported.
