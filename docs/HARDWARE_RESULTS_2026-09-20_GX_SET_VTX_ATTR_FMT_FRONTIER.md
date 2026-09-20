# Hardware result — #199 pass, GXSetVtxAttrFmt frontier

Date: 2026-09-20
Tracking: #117, #154, #162
Baseline: main `00708f4140255c1792ddc7fc0fc870cae2934c76` (#199)

## Result

The first hardware run after #199 explicitly records:

```text
TaskThread::run hits  : 1
GXSetProjection hits  : 1
GXSetViewport hits    : 1
GXSetScissor hits     : 1
GXLoadPosMtxImm hits  : 1
GXSetCurrentMtx hits  : 1
GXClearVtxDesc hits   : 1
GXSetVtxDesc hits     : 1
```

Therefore merged #199 is hardware-crossed. The durable scheduler snapshot
returns to default/main `0x80347498`.

## New exact blocker

Hardware now stops at:

```text
kind   : DIRECT
target : 0x8016DC68
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_VTX_DESC
```

At pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`, `0x8016DC68` is
`GXSetVtxAttrFmt`.

The observed `r3 = 0` identifies `GX_VTXFMT0`. The blocker diagnostic does
not record `r4..r7`, so the concrete attribute/count/type/fraction values are
not inferred from this run.

## Pinned semantics

Pinned `GXSetVtxAttrFmt` consumes:

```text
r3 = vtxfmt
r4 = attr
r5 = component count
r6 = component type
r7 = fractional bits
```

It canonicalizes NBT to NRM for tracked HLE state, updates
`g_hleGxState.vtxAttrFmt[vtxfmt][attr]`, clears the separate NBT slot after
canonicalization, invalidates the cached vertex-layout hash only if count/type/
fraction actually changed, then forwards valid public attributes to Aurora
`GXSetVtxAttrFmt`.

## Graphics/resource state

The rendered graphics report remains at nine FIFO writes. The ninth byte is:

```text
FIFO EVENT #9 size=1 value=0x00000048
```

Pinned Aurora emits `0x48` from `GXInvalidateVtxCache()`. This remains the
first observed post-bootstrap GX/vertex-state FIFO traffic. It is not drawable
work yet.

The durable snapshot reports:

```text
display-list calls    : 0
FIFO produced work    : NO
GXCopyDisp calls      : 0
present successes     : 0
present failures      : 0
```

FST publication remains valid at `0x97DC0000`, size 64,224 bytes, 2,096
entries. No `dvd-read-status.txt` was produced.

## Next hardware acceptance

After the GXSetVtxAttrFmt bridge is merged:

1. `GXSetVtxAttrFmt hits` must become non-zero;
2. `0x8016DC68` must no longer record a DIRECT blocker;
3. preserve TaskThread and the existing GX hit chain;
4. preserve scheduler recovery;
5. watch for additional vertex-state FIFO traffic;
6. capture the next exact GX/resource/DVD boundary;
7. separately watch for first display list, drawable FIFO work,
   `GXCopyDisp`, or successful present.
