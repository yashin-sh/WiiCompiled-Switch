# Hardware result — #198 pass, GXSetVtxDesc frontier

Date: 2026-09-20  
Tracking: #117, #154, #162  
Baseline: main `75361316dd306409f90478919375b74653eddbe7` (#198)

## Result

The first hardware run after #198 explicitly records:

```text
TaskThread::run hits  : 1
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
GXSetCurrentMtx hits  : 1
GXClearVtxDesc hits   : 1
```

Therefore merged #198 is hardware-crossed and the priority-24 TaskThread is
independently re-proved. The durable scheduler snapshot returns to default/main
`0x80347498`.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x8016D3A4
r1     : 0x80399008
r3     : 0x00000009
stage  : RMCP01_GX_CLEAR_VTX_DESC
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x8016D3A4` is
`GXSetVtxDesc`.

The observed `r3 = 9` identifies `GX_VA_POS`. The durable blocker file does
not record `r4`, so the concrete descriptor type is intentionally not inferred
from this run.

## Pinned semantics

Pinned `GXSetVtxDesc`:

1. canonicalizes `GX_VA_NBT` to `GX_VA_NRM` for tracked HLE state;
2. ignores invalid / null attributes;
3. records the descriptor type and invalidates the vertex-layout hash only on
   an actual state change;
4. keeps `GX_VA_NBT` itself cleared after NBT canonicalization;
5. records matrix-index descriptors in HLE state without forwarding them to
   Aurora;
6. forwards normal attributes to Aurora `GXSetVtxDesc`;
7. expands `GX_INDEX8` / `GX_INDEX16` into `GX_DIRECT` on the Aurora
   streaming side, matching the pinned decoder strategy.

## First post-bootstrap GX FIFO state traffic

The rendered graphics report now records a ninth FIFO byte after the historical
eight video-bootstrap writes:

```text
FIFO EVENT #9 size=1 value=0x00000048
```

Pinned Aurora defines `GXInvalidateVtxCache()` as a single `0x48` GX FIFO
byte. This is the first observed post-bootstrap GX/vertex-state FIFO traffic in
the rendered RMCP01 run. It is not yet drawable work and does not constitute a
frame.

The durable snapshot is taken before the completed `GXClearVtxDesc` call, so
its embedded FIFO counters still show the previous eight-write state. The
renderer report is the later source of truth for the ninth event.

## Graphics/resource state

No display list, drawable FIFO work, `GXCopyDisp`, or present is recorded yet.
The renderer remains initialized/frame-active.

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` was produced for this run.

## Scheduler/thread state

The priority-6 worker `0x90112660` still enters its guest fiber and returns
WAITING on VI queue `0x80386BC0`. The priority-24 ResourceManager TaskThread
also reaches guest-fiber entry, and the durable snapshot returns to the
default/main thread.

## Next hardware acceptance

After the GXSetVtxDesc bridge is merged:

1. `GXSetVtxDesc hits` must become non-zero;
2. `0x8016D3A4` must no longer record a DIRECT blocker;
3. preserve the full projection/viewport/scissor/matrix/descriptor hit chain;
4. preserve TaskThread and default/main scheduler recovery;
5. watch whether vertex configuration produces additional FIFO state traffic;
6. capture the next exact GX/resource/DVD boundary;
7. separately watch for first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
