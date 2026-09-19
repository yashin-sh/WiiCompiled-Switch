# Hardware result — #194 pass, GXSetScissor frontier

Date: 2026-09-19  
Tracking: #117, #154, #162  
Baseline: main `b0d6cd7f0920e4426395dadb2c39718d788c6136` (#194)

## Result

The first hardware run after #194 explicitly records:

```text
GXSetProjection hits : 1
GXSetViewport hits   : 1
```

Therefore merged #194 is hardware-crossed: the viewport bridge executes on
real Switch hardware and the old `0x801733B4` blocker is gone.

The durable snapshot at the new frontier records:

```text
translated dispatches : 2375
post-main dispatches  : 1769
VIWaitForRetrace      : 13
PostRetrace callbacks : 114
OSReceiveMessage      : 2
OSSleepThread         : 3
OSWakeupThread        : 231
SelectThread          : 29
current/running       : 0x80347498 / 0x80347498
```

The scheduler remains recovered on the default/main thread.

## TaskThread note

This particular run records `TaskThread::run hits = 0`, so it does not
re-prove #192 by counter. It does still create/resume the priority-24
`0x8042E480` worker and reaches its guest-fiber entry. The previous hardware
run already recorded `TaskThread::run hits = 1`; this run contains no evidence
of a TaskThread/scheduler regression.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x80173430
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_VIEWPORT
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x80173430` is
`GXSetScissor`.

Pinned behavior consumes unsigned `left/top/width/height` from PPC
`r3..r6`, updates the guest GX scissor BP words at `GXData + 0x148` and
`GXData + 0x14C`, clears the guest GX dirty halfword at `GXData + 2`, then
forwards the rectangle to Aurora `GXSetScissor`.

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

The eight writes remain the same bootstrap BP traffic.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` is produced, so the local DVD read bridge
remains installed but hardware-unreached.

## Next hardware acceptance

After the GXSetScissor bridge is merged:

1. `GXSetScissor hits` must become non-zero;
2. `0x80173430` must no longer record a DIRECT blocker;
3. preserve `GXSetProjection hits > 0` and `GXSetViewport hits > 0`;
4. preserve scheduler recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for the first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
