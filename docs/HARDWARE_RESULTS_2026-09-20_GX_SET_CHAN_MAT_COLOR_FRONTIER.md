# Hardware result — #201 pass, GXSetChanMatColor frontier

Date: 2026-09-20
Tracking: #117, #154, #162
Baseline: main `dcf3c037f12552edf7ffaf0703a16fb426b56c0b` (#201)

## Result

The first hardware run after #201 explicitly records:

```text
TaskThread::run hits  : 1
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
GXSetCurrentMtx hits  : 1
GXClearVtxDesc hits   : 1
GXSetVtxDesc hits     : 1
GXSetVtxAttrFmt hits  : 1
GXSetNumChans hits    : 1
```

Therefore merged #201 is hardware-crossed. The durable scheduler snapshot
returns to default/main `0x80347498`.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x80170474
r1     : 0x80399008
r3     : 0x00000004
stage  : RMCP01_GX_SET_NUM_CHANS
action : abort after durable blocker record
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x80170474` is
`GXSetChanMatColor`.

Pinned semantics consume:

```text
r3 = GXChannelID
r4 = guest pointer to one packed RGBA word
```

The runtime ensures an Aurora frame is active, reads the 32-bit guest color,
decodes it as RGBA bytes, then forwards it to Aurora `GXSetChanMatColor`.

The observed `r3 = 4` is preserved as hardware evidence. The blocker file does
not record `r4`, so the concrete guest color pointer is not inferred.

## Graphics/resource state

The renderer remains initialized/frame-active with nine FIFO writes. The ninth
byte remains `0x48`, the first post-bootstrap GX vertex-state FIFO event.

There is still no display list, drawable FIFO work, `GXCopyDisp`, or present.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No DVD-read status file is produced.

## Next hardware acceptance

After the GXSetChanMatColor bridge is merged:

1. `GXSetChanMatColor hits` must become non-zero;
2. `0x80170474` must no longer record a DIRECT blocker;
3. preserve `GXSetNumChans` and the existing GX hit chain;
4. preserve TaskThread and scheduler recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
