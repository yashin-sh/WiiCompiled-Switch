# Hardware result — #190 OSSendMessage pass and GXDrawDone frontier

Date: 2026-09-19  
Tracking: #117, #162  
Baseline: main `8f263297d87fa5f50627997ef79b98f99662427e` (#190)

## Result

The first hardware run after #190 crosses the previous PAL
`OSSendMessage (0x801A735C)` blocker. The scheduler recovery from #189 remains
intact:

```text
initial thread  : 0x8042A680 -> WAITING on 0x804294F8
worker thread   : 0x90112660 priority 6
worker state    : WAITING on 0x80386BC0
main/current    : 0x80347498
```

The independent watchdog remains ACTIVE while translated dispatches advance.

The durable snapshot reaches 2,277 translated dispatches / 1,671 post-main
dispatches and records:

```text
VIWaitForRetrace : 11
PostRetrace      : 112
OSReceiveMessage : 2
OSSleepThread    : 2
OSWakeupThread   : 226
SelectThread     : 23
RKSystem::run    : 0
StaticR          : 0
```

## New exact blocker

```text
kind   : DIRECT
target : 0x8016EAB0
r3     : 0x8042944C
stage  : HOST_CONTEXT_SWITCH_RETURNED
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`,
`0x8016EAB0` is `GXDrawDone`
(`GX__DrawDone_8016eab0`).

Pinned behavior:

1. clear guest draw-done flag at `0x803867D8`;
2. call Aurora `GXDrawDone()`, which drains the GX FIFO and invokes the
   host-side draw-done callback if installed;
3. publish the PE-finish bookkeeping by ORing `0x0008` into
   `GXData + 0x0A`;
4. set the guest draw-done flag back to 1.

The Switch implementation mirrors that contract. Only the rendered fast-track
calls Aurora's real `GXDrawDone()`; synthetic/headless builds retain the
guest-visible bookkeeping seam without introducing a renderer dependency.

## Graphics state

The renderer remains initialized and frame-active, but RMCP01 still emits only
the same eight video-bootstrap FIFO writes:

```text
RMCP01 FIFO writes : 8
display-list calls : 0
FIFO produced work : NO
GXCopyDisp calls   : 0
present successes  : 0
```

This `GXDrawDone` boundary is therefore a real GX synchronization frontier,
not yet proof of a drawable game frame.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No DVD read frontier is identified by this run.

## Next hardware acceptance

After the GXDrawDone bridge is merged:

1. cross `0x8016EAB0` without an unsupported DIRECT abort;
2. preserve the #189/#190 scheduler/message-queue liveness;
3. capture the next exact blocker;
4. note separately whether drawable FIFO work, display lists, `GXCopyDisp`,
   `RKSystem::run`, StaticR or DVD reads become non-zero.
