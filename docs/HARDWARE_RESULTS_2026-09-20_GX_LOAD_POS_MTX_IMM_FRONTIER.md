# Hardware result — #195 pass, GXLoadPosMtxImm frontier

Date: 2026-09-20  
Tracking: #117, #154, #162  
Baseline: main `723f9a64b4d191f3d8ab66edecec3a228eefa5c1` (#195)

## Result

The first hardware run after #195 explicitly records:

```text
GXSetProjection hits : 1
GXSetViewport hits   : 1
GXSetScissor hits    : 1
```

Therefore merged #195 is hardware-crossed: the scissor bridge executes on
real Switch hardware and the old `0x80173430` blocker is gone.

The durable snapshot at the new frontier records:

```text
translated dispatches : 2350
post-main dispatches  : 1744
VIWaitForRetrace      : 13
PostRetrace callbacks : 109
OSReceiveMessage      : 2
OSSleepThread         : 3
OSWakeupThread        : 221
SelectThread          : 29
current/running       : 0x80347498 / 0x80347498
```

The scheduler remains recovered on the default/main thread.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x8017310C
r1     : 0x80399008
r3     : 0x80399018
stage  : RMCP01_GX_SET_SCISSOR
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x8017310C` is
`GXLoadPosMtxImm`.

Pinned behavior consumes the guest matrix address from PPC `r3` and matrix id
from `r4`, converts twelve big-endian float32 values from the guest 3x4
position matrix into host floats, then forwards the matrix unchanged to Aurora
`GXLoadPosMtxImm`.

## Graphics/resource state

The renderer remains initialized and frame-active, but RMCP01 still reports:

```text
FIFO writes          : 8
display-list calls   : 0
FIFO produced work   : NO
GXCopyDisp calls     : 0
present successes    : 0
present failures     : 0
```

The eight writes remain the same bootstrap BP traffic; no drawable FIFO work
has appeared yet.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` is produced, so the local DVD read bridge
remains installed but hardware-unreached.

## TaskThread note

This particular run records `TaskThread::run hits = 0`. The previous hardware
proof of `TaskThread::run hits = 1` remains valid, and this run's thread log
still shows the priority-24 worker entering its guest fiber. No scheduler
regression is indicated.

## Next hardware acceptance

After the GXLoadPosMtxImm bridge is merged:

1. `GXLoadPosMtxImm hits` must become non-zero;
2. `0x8017310C` must no longer record a DIRECT blocker;
3. preserve `GXSetProjection/Viewport/Scissor hits > 0`;
4. preserve default/main scheduler recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for the first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
