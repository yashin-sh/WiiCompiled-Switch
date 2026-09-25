# Hardware result — GXLoadTexObj crossed / GXSetTexCoordGen2 frontier (2026-09-25)

Tracking: #117, #154, #162

## GXLoadTexObj crossing

The rendered hardware run records:

```text
status=load-pass
obj=0x901136B4
tid=0
data=0x00F103E0
width=832
height=456
format=4
wrap_s=0
wrap_t=0
mipmap=0
size=0x000B9400
```

That status is followed by durable execution to a distinct unsupported direct
boundary, so the first `GXLoadTexObj (0x80170F2C)` is hardware-crossed.

The later durable snapshot reaches 3,363 translated dispatches / 2,757
post-main dispatches. It preserves:

- `RKSystem::run hits = 1`;
- `TaskThread::run hits = 2`;
- FST valid at 64,224 bytes / 2,096 entries;
- `English.szs` read-pass at 299,969 bytes;
- `/rel/StaticR.rel` read-pass at 4,903,876 bytes;
- 29 RMCP01 FIFO writes with produced work;
- `GXCopyDisp calls = 1`;
- one successful present and zero failures.

The later GX counters also increase beyond the prior run:
GXSetProjection/Viewport/Scissor/LoadPosMtxImm/SetCurrentMtx reach two hits,
GXSetVtxDesc reaches three, and GXSetVtxAttrFmt reaches three.

## New exact blocker

```text
kind   = DIRECT
target = 0x8016E37C
r3     = 0
r4     = 1
r5     = 4
r6     = 60
r7     = 0
r8     = 125
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x8016E37C` exactly to:

```cpp
GX__SetTexCoordGen2_8016e37c(
    uint32_t dc,
    uint32_t f,
    uint32_t sp,
    uint32_t m,
    uint32_t n,
    uint32_t pm)
```

The live arguments decode to:

```text
dst       = GX_TEXCOORD0
type      = GX_TG_MTX2x4
src       = GX_TG_TEX0
mtx       = GX_IDENTITY
normalize = GX_FALSE
postMtx   = GX_PTIDENTITY
```

This matches Aurora's default identity texture-coordinate generator shape.

## Exact candidate

The candidate accepts only those six hardware-observed values and forwards:

```cpp
GXSetTexCoordGen2(
    GX_TEXCOORD0,
    GX_TG_MTX2x4,
    GX_TG_TEX0,
    GX_IDENTITY,
    GX_FALSE,
    GX_PTIDENTITY);
```

Any later argument variation aborts as
`GX_SET_TEX_COORD_GEN2_UNPROVEN_ARGS` and becomes a fresh hardware frontier.

No GXSetTexCoordGen neighbor, texture-matrix upload, array function,
GXEnableTexOffsets, line/point setup, or unrelated GX API is pre-ported.

## Next acceptance

A future run must durably progress beyond `0x8016E37C` to count this
candidate as crossed. A new blocker, later milestone, or attributable native
exception defines the next step.
