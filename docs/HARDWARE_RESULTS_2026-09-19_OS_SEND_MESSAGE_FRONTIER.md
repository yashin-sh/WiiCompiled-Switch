# Hardware result — #189 scheduler recovery and OSSendMessage frontier

Date: 2026-09-19  
Tracking: #117, #162  
Baseline: main `052a0832c5497685911ce0501a30166fb19389e6` (#189)

## Result

The first hardware run after #189 validates the two pending VI/scheduler gates.

The initial HostContext-backed guest thread returns to the historically proven
blocking message path:

```text
thread      : 0x8042A680
entry       : 0x8024373C  EGG::Thread::start(void*)
entry arg   : 0x804294E4
resume state: WAITING
queue       : 0x804294F8
```

The later priority-6 worker also now behaves correctly:

```text
thread      : 0x90112660
entry       : 0x8024373C
arg         : 0x8042E930
priority    : 6
resume state: WAITING
queue       : 0x80386BC0  VI retrace queue
```

This is the hardware proof that the previous guest starvation is fixed. The
final scheduler snapshot returns to the default/main thread `0x80347498`,
priority 16.

## Progress counters

The final durable post-main snapshot records:

```text
translated dispatches : 2581
post-main dispatches  : 1975
VIWaitForRetrace      : 13
PostRetrace callbacks : 158
OSReceiveMessage      : 2
OSSleepThread         : 2
OSWakeupThread        : 318
SelectThread          : 27
RKSystem::run          : 0
StaticR                : 0
```

The game therefore progresses substantially beyond the #188 first-fiber crash
and the old durable priority-6 starvation state, but has not yet reached
`RKSystem::run` or StaticR.

## New exact blocker

The run terminates on a new explicit translated native boundary:

```text
kind   : DIRECT
target : 0x801A735C
r3     : 0x8042BBFC
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`,
`0x801A735C` is `OSSendMessage`
(`OS__SendMessage_HLE_801a735c`).

Pinned semantics use the same `OSMessageQueue` layout already implemented for
`OSReceiveMessage`:

- disable interrupts once for the operation;
- if the queue has space, append the message to the ring buffer;
- wake receivers on the embedded receive queue at `queue + 0x08`;
- restore interrupts and return 1;
- if full and non-blocking, restore interrupts and return 0;
- if full and blocking, park on the embedded send queue at `queue + 0x00`
  through `OSSleepThread`, then retry.

The Switch implementation mirrors only this pinned contract and reuses the
already hardware-proven `OSSleepThread` / `OSWakeupThread` scheduler path.

## Unchanged subsystems

The renderer remains initialized and active, but the game still emits only the
same eight video-bootstrap FIFO writes:

```text
RMCP01 FIFO writes : 8
display-list calls : 0
FIFO produced work : NO
GXCopyDisp calls   : 0
present successes  : 0
```

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No DVD read status file is produced.

Therefore the next patch is limited to `OSSendMessage`; no VI priority,
DVD/FST, Aurora, Dawn, Vulkan/NVK or GX changes are justified by this run.

## Next hardware acceptance

After the `OSSendMessage` bridge is merged, the next run must:

1. cross `0x801A735C` without an unsupported-dispatch abort;
2. preserve scheduler/message-queue liveness;
3. report the next exact blocker;
4. separately note if `RKSystem::run`, StaticR, DVD reads or real drawable GX
   work become non-zero.
