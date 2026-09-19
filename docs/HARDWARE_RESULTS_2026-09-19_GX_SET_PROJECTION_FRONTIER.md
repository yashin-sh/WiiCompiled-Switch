# Hardware result — #192 run, GXSetProjection frontier

Date: 2026-09-19  
Tracking: #117, #154, #162  
Baseline: main `b15c5d1208054aa753756dd09c4415855a28c844` (#192)

## Result

The first hardware run after #192 no longer stops at the previous
`INDIRECT_JUMP_MISS 0x80242D7C`. The priority-24 resource TaskThread still
enters its guest fiber, and overall execution advances substantially farther.

The durable snapshot reaches:

```text
translated dispatches : 4107
post-main dispatches  : 3501
PostRetrace callbacks : 417
OSWakeupThread        : 836
SelectThread          : 23
current/running       : 0x80347498 / 0x80347498
```

The scheduler is therefore still live and returns to the default/main thread.

## TaskThread validation caveat

The bounded thread log still records the priority-24 object:

```text
thread    : 0x8042E480
entry     : 0x8024373C
arg       : 0x8042BBF0
priority  : 24
vtable    : 0x802A3F90
vt_run    : 0x80242D7C
```

and the fiber-entry event is present.

However this run predates a dedicated `TaskThread::run` dispatch counter, and
`OSReceiveMessage` remains at 2. Therefore absence of the old blocker is not
by itself sufficient to claim that a queued TaskThread job executed. The next
diagnostic revision adds explicit `TaskThread::run hits` and
`GXSetProjection hits` counters.

## New exact blocker

```text
kind   : DIRECT
target : 0x8017301C
r1     : 0x80399008
r3     : 0x80399048
stage  : RMCP01_GX_DRAW_DONE
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x8017301C` is
`GXSetProjection`.

Pinned behavior reads the guest big-endian 4x4 matrix, converts the 16 entries
to host floats, and forwards the matrix plus `GXProjectionType` to Aurora GX.
RMCP01's EGG ProcessMeter also references this exact PAL GX entry point, making
this a concrete game-facing GX transform boundary.

## Graphics/resource state

The renderer remains initialized and frame-active, but RMCP01 still emits only
the same eight bootstrap FIFO writes:

```text
FIFO writes          : 8
display-list calls   : 0
FIFO produced work   : NO
GXCopyDisp calls     : 0
present successes    : 0
```

The FST remains valid at `0x97DC0000`, size 64,224 bytes, 2,096 entries.
No `dvd-read-status.txt` is produced, so the local DVD read bridge remains
installed but unreached.

## Next hardware acceptance

After the GXSetProjection bridge is merged:

1. `0x8017301C` must no longer record a DIRECT blocker;
2. `GXSetProjection hits` must become non-zero;
3. `TaskThread::run hits` will explicitly determine whether #192 is reached;
4. preserve scheduler liveness and main-thread recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately record any first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
