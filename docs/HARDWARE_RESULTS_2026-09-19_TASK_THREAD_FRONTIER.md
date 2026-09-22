# Hardware result — #191 GXDrawDone pass and TaskThread resource frontier

Date: 2026-09-19  
Tracking: #117, #154, #162  
Baseline: main `d3e0f4406be6f666d7763b159e63038603d37010` (#191)

## Result

The first hardware run after #191 crosses the previous PAL
`GXDrawDone (0x8016EAB0)` blocker. The scheduler remains healthy and the
renderer remains initialized.

The durable runtime snapshot records:

```text
translated dispatches : 2283
post-main dispatches  : 1677
VIWaitForRetrace      : 11
PostRetrace callbacks : 113
OSReceiveMessage      : 2
OSSleepThread         : 2
OSWakeupThread        : 228
SelectThread          : 23
current/running       : 0x80347498 / 0x80347498
```

The independent watchdog remains ACTIVE while dispatches advance from 607 to
817 to 1384.

## New exact blocker

```text
kind   : INDIRECT_JUMP_MISS
target : 0x80242D7C
pc     : 0x8024373C
r1     : 0x8042E458
r3     : 0x8042BBF0
stage  : HOST_CONTEXT_SWITCH_ENTER
```

The bounded thread log identifies this object before the miss:

```text
thread    : 0x8042E480
entry     : 0x8024373C  EGG::Thread::start(void*)
arg       : 0x8042BBF0
priority  : 24
vtable    : 0x802A3F90
vt_run    : 0x80242D7C
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x80242D7C` is the native
override `TaskThread_run_HLE_80242d7c` for `EGG::TaskThread::run()`.

RMCP01 source attribution matches the runtime object: `System::ResourceManager`
creates an `EGG::TaskThread` with priority 24 and uses it for asynchronous
resource jobs. This run therefore reaches the resource worker itself, although
no `DVDReadPrio` / `DVDReadAsyncPrio` request has yet been observed.

## Pinned TaskThread contract

The pinned worker:

1. seeds GQR2-GQR5;
2. blocks on the TaskThread message queue through `OSReceiveMessage`;
3. publishes the current job pointer;
4. dispatches the job callback with its argument;
5. optionally dispatches the job completion callback;
6. optionally sends the job token to the done queue with `OSSendMessage`;
7. clears the retired job slot and loops.

The pin also includes the MovieManager/THP prepare special case; the Switch
bridge mirrors it rather than introducing a ResourceManager-only shortcut.

Because the hardware miss is a virtual indirect jump, the Switch indirect
resolver must recognize this native boundary before consulting the generated
translated-function table.

## Graphics/resource state

The renderer remains initialized and frame-active but RMCP01 still emits only
the same eight video-bootstrap FIFO writes. There is still no display list,
drawable FIFO work, `GXCopyDisp`, or present.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` is produced by this run.

## Next hardware acceptance

After the TaskThread native bridge is merged:

1. cross virtual target `0x80242D7C` without `INDIRECT_JUMP_MISS`;
2. preserve scheduler liveness and the VI worker wait behavior;
3. capture the first resource-job callback or next exact blocker;
4. record whether `DVDReadPrio` / `DVDReadAsyncPrio` are finally reached;
5. separately record any new drawable FIFO / display-list / `GXCopyDisp`
   activity.

## Later attribution correction

Follow-up hardware telemetry on 2026-09-22 shows that this exact priority-24
TaskThread object has `mJobCount=5` and `mStackSize=0x2800`. That concrete
runtime shape does not match the currently decompiled
`System::ResourceManager` TaskThread creation shape. The historical
ResourceManager attribution above is therefore superseded for this exact
object; the hardware fact that remains valid is only that it is the
priority-24 `EGG::TaskThread` at `0x8042BBF0`.

The later send-side trace also proves `TaskThread::request` sends a valid
`mJobs[]` pointer. The active frontier is now the blocking
`OSReceiveMessage` output-slot clobber, not callback mapping.
