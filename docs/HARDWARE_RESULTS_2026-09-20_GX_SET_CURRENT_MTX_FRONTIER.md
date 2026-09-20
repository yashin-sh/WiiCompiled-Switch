# Hardware result — #196 pass, GXSetCurrentMtx frontier

Date: 2026-09-20  
Tracking: #117, #154, #162  
Baseline: main `d66d36043318b7fef47d53955d36b4393b10d93c` (#196)

## Result

The first hardware run after #196 explicitly records:

```text
TaskThread::run hits  : 1
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
```

Therefore merged #196 is hardware-crossed and this run also re-proves the
priority-24 TaskThread path. The durable snapshot remains scheduler-healthy with
default/main current/running at `0x80347498`.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x80173214
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_LOAD_POS_MTX_IMM
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x80173214` is
`GXSetCurrentMtx`.

Pinned behavior consumes only the PPC `r3` matrix id and forwards it directly
to Aurora `GXSetCurrentMtx(id)`.

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

The eight writes remain the same video-bootstrap BP traffic; no drawable FIFO
work has appeared yet.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` is produced, so the local DVD read bridge
remains installed but hardware-unreached.

## Scheduler/thread state

The priority-6 worker still enters its guest fiber and returns WAITING on VI
queue `0x80386BC0`. The priority-24 ResourceManager TaskThread also enters its
guest fiber, and the durable snapshot returns to default/main
`0x80347498`.

## Next hardware acceptance

After the GXSetCurrentMtx bridge is merged:

1. `GXSetCurrentMtx hits` must become non-zero;
2. `0x80173214` must no longer record a DIRECT blocker;
3. preserve the projection/viewport/scissor/position-matrix hit chain;
4. preserve TaskThread and default/main scheduler recovery;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
