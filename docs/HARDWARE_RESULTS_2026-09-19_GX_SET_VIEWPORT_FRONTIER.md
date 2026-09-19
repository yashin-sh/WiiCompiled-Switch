# Hardware result — #193 pass, GXSetViewport frontier

Date: 2026-09-19  
Tracking: #117, #154, #162  
Baseline: main `280284a4df45c7364ed918be603e684aa92e0f8c` (#193)

## Result

The first hardware run after #193 proves both previously ambiguous boundaries:

```text
TaskThread::run hits : 1
GXSetProjection hits : 1
```

Therefore merged #192 and #193 are both hardware-crossed.

The durable snapshot at the projection boundary records:

```text
translated dispatches : 2390
post-main dispatches  : 1784
VIWaitForRetrace      : 14
PostRetrace callback  : 114
OSReceiveMessage      : 2
OSSleepThread         : 3
OSWakeupThread        : 231
SelectThread          : 31
current/running       : 0x80347498 / 0x80347498
```

The scheduler remains recovered on the default/main thread.

## New exact blocker

After returning from the bridged projection call, hardware stops at:

```text
kind   : DIRECT
target : 0x801733B4
r1     : 0x80399008
r3     : 0x80399048
stage  : RMCP01_GX_SET_PROJECTION
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x801733B4` is
`GXSetViewport`.

The pinned native ABI declares six scalar float parameters. PPC EABI therefore
passes them in `f1..f6`, and WiiCompiled forwards those six values directly to
Aurora `GXSetViewport(left, top, width, height, nearZ, farZ)`.

## Graphics/resource state

The graphics backend remains initialized and frame-active. RMCP01 still has:

```text
FIFO writes        : 8
display-list calls : 0
FIFO produced work : NO
GXCopyDisp calls   : 0
present successes  : 0
```

The eight writes remain the same video/bootstrap BP traffic. No real drawable
RMCP01 FIFO work has appeared yet.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` is produced, so the local DVD read bridge
remains installed but hardware-unreached.

## Next hardware acceptance

After the GXSetViewport bridge is merged:

1. `GXSetViewport hits` must become non-zero;
2. `0x801733B4` must no longer record a DIRECT blocker;
3. preserve the now-proven TaskThread and GXSetProjection crossings;
4. preserve default/main scheduler recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for the first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
