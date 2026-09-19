# Hardware result — #188 VI poll first-fiber context regression

Date: 2026-09-19  
Tracking: #117, #162  
Baseline: main `5c1ddc190330a59e36d70bc27dadbcae73f6d44a` (#188)

## Summary

The first real-Switch run after #188 crashes after a short black screen. The
crash is earlier than the durable priority-6 worker that #188 was intended to
fix, so this run cannot yet prove or disprove the starvation correction.

The exact durable blocker is:

```text
kind             : GUEST_FIBER_ENTRY_EXCEPTION
target / guest pc: 0x8024373C  EGG::Thread::start(void*)
stage            : HOST_CONTEXT_SWITCH_ENTER
thread           : 0x8042A680
entry arg        : 0x804294E4
fiber r1         : 0x8042A658
fiber r3         : 0x804294E4
```

The lifecycle log proves the new guest fiber receives the expected entry
argument before translated dispatch. Historical hardware runs had already
crossed this same `0x8042A680 -> 0x8024373C` path far enough to reach
`OSReceiveMessage` and `OSSleepThread`.

## State at the crash

The latest durable translated snapshot reports:

```text
dispatch count        : 901
post-main dispatch    : 295
RKSystem::run hits    : 0
StaticR dispatches    : 0
VIWaitForRetrace hits : 2
OSSleepThread hits    : 0
OSWakeupThread hits   : 2
SelectThread hits     : 0
OSLoadContext hits    : 0
```

The graphics path is unchanged from the earlier bootstrap frontier:

```text
renderer initialized  : YES
renderer frame active : YES
RMCP01 FIFO writes    : 8
display-list calls    : 0
FIFO produced work    : NO
GXCopyDisp calls      : 0
present successes     : 0
```

The local FST also remains published at `0x97DC0000`, size 64,224 bytes,
2,096 entries. No DVD-read status file is produced. This excludes the local
FST/DVD bridge and the Aurora/Dawn/NVK renderer as the immediate cause.

## Regression attribution

#188 added `mkw_switch_hle_vi_poll_retrace(cpu)` to translated runtime call
boundaries. That poll may call `AdvanceRetrace`, which intentionally uses the
provided register file for guest-visible interrupt work:

- `r3 = 0x80386BC0` for `OSWakeupThread`;
- `r3 = retraceValue` for pre/post-retrace callbacks.

At a runtime call boundary, however, the same `CpuContext` also contains the
next translated callee's ABI arguments. In this failing case `r3=0x804294E4`
is the EGG object passed to `EGG::Thread::start`.

Pinned WiiCompiled already documents the required distinction: retrace/alarm
service that interrupts translated execution must use interrupt-context
isolation so callback register mutations cannot leak back into the interrupted
function. The Switch runtime-boundary poll lacked that isolation.

## Minimal correction

The Switch VI poll now wraps its active-fiber service in a scoped full
`CpuContext` restore. Retrace state, guest memory writes, wait queues and guest
scheduler effects are preserved, while temporary callback/wakeup register
values are discarded when the poll returns to the interrupted translated call
boundary.

This intentionally does **not** change:

- guest priorities;
- `VIWaitForRetrace` queue semantics introduced by #188;
- local DVD/FST behavior;
- Aurora, Dawn, Vulkan/NVK or GX behavior.

Public synthetic CI retains the poll seam, but the active HostContext case is
hardware-only because public CI contains no RMCP01 guest thread.

## Next hardware acceptance

The next rendered run must first prove the regression is gone:

```text
0x8042A680
-> EGG::Thread::start (0x8024373C)
-> OSReceiveMessage
-> OSSleepThread
```

Only after that path is restored should #188's original starvation acceptance
be evaluated:

```text
0x90112660 RUNNING prio 6
-> VIWaitForRetrace
-> OSSleepThread(0x80386BC0)
-> WAITING
-> SelectThread increases
-> default/main runs between retraces
```

If the first-fiber exception remains, the next change must be driven by the new
exact blocker rather than by speculative scheduler, DVD or renderer work.
