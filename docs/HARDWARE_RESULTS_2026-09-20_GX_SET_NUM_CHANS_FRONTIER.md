# Hardware result — #200 pass, GXSetNumChans frontier

Date: 2026-09-20
Tracking: #117, #154, #162
Baseline: main `7bf0d8b5458c1748cb301860e7e553325b63f984` (#200)

## Result

The first hardware run after #200 explicitly records:

```text
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
GXSetCurrentMtx hits  : 1
GXClearVtxDesc hits   : 1
GXSetVtxDesc hits     : 1
GXSetVtxAttrFmt hits  : 1
```

Therefore merged #200 is hardware-crossed. This particular snapshot records
`TaskThread::run hits = 0`, but the priority-24 TaskThread still reaches
guest-fiber entry in the thread-event log, so the prior TaskThread hardware
proof is not revoked. The scheduler returns to default/main `0x80347498`.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x8017054C
r1     : 0x80399008
r3     : 0x00000001
stage  : RMCP01_GX_SET_VTX_ATTR_FMT
action : abort after durable blocker record
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x8017054C` is
`GXSetNumChans`.

Pinned semantics are exactly:

```cpp
GXSetNumChans((u8)n);
```

The observed `r3 = 1` therefore requests one GX color/lighting channel.

## User-visible termination

The user observed a black screen for roughly 15–25 seconds, then a return to
hbmenu followed by a Switch error message.

This matches the current blocker-driven fast-track design: an unsupported direct
native dispatch is first written durably, then `InvokeDirectCpu` calls
`std::abort()`. The durable blocker file confirms that exact path. This is
therefore consistent with the deliberate diagnostic stop, not evidence by
itself of an Aurora/NVK GPU crash.

No `fast-track-exception.txt` was supplied for this run.

## Graphics/resource state

The renderer initializes successfully and remains at nine FIFO writes. The ninth
byte remains `0x48`, the first post-bootstrap GX/vertex-state FIFO traffic.

There is still no display list, drawable FIFO work, `GXCopyDisp`, or present.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No DVD-read status file is produced.

## Next hardware acceptance

After the GXSetNumChans bridge is merged:

1. `GXSetNumChans hits` must become non-zero;
2. `0x8017054C` must no longer record a DIRECT blocker;
3. preserve the existing GX hit chain;
4. preserve scheduler recovery and TaskThread lifecycle;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
