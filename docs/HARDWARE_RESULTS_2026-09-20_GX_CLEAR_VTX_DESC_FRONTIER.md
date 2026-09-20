# Hardware result — #197 pass, GXClearVtxDesc frontier

Date: 2026-09-20  
Tracking: #117, #154, #162  
Baseline: main `7ce294f5ed7b10a32900a598852375c57c679382` (#197)

## Result

The first hardware run after #197 explicitly records:

```text
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
GXSetCurrentMtx hits  : 1
```

Therefore merged #197 is hardware-crossed. This particular run records
`TaskThread::run hits = 0`; the previous hardware proof remains valid, and the
priority-24 ResourceManager TaskThread still reaches guest-fiber entry. The
durable snapshot remains scheduler-healthy with default/main current/running at
`0x80347498`.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x8016DC34
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_CURRENT_MTX
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x8016DC34` is
`GXClearVtxDesc`.

Pinned behavior:

1. checks all 26 tracked vertex descriptors;
2. clears each descriptor to `GX_NONE`;
3. invalidates the cached vertex-layout hash only if descriptor state changed;
4. deliberately preserves vertex-array base/stride state;
5. forwards `GXClearVtxDesc()` to Aurora GX.

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
queue `0x80386BC0`. The priority-24 ResourceManager TaskThread also reaches
guest-fiber entry. The durable snapshot returns to default/main
`0x80347498`.

## Next hardware acceptance

After the GXClearVtxDesc bridge is merged:

1. `GXClearVtxDesc hits` must become non-zero;
2. `0x8016DC34` must no longer record a DIRECT blocker;
3. preserve the projection/viewport/scissor/position/current-matrix hit chain;
4. preserve scheduler recovery and TaskThread lifecycle behavior;
5. capture the next exact GX/resource/DVD boundary;
6. separately watch for first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
