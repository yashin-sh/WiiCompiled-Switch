# PAL GXSetTevOp — 0x80171C4C

Tracking: #117, #162

## Hardware blocker

The rendered run after merged #208 reaches:

```text
kind   : DIRECT
target : 0x80171c4c
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_NUM_TEV_STAGES
```

Because that stage is written inside the implemented GXSetNumTevStages bridge,
the later exact target is hardware proof that the previous boundary returned
and execution advanced.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetTevOp_80171c4c(uint32_t s, uint32_t m) {
    if (!TevStageOk(s)) return;
    GXSetTevOp((GXTevStageID)s, (GXTevMode)m);
}
```

`TevStageOk(s)` requires `s < GX_MAX_TEVSTAGE`.

The blocker captured only `r3 = 0`; its diagnostic format does not include
`r4`. Therefore no hardware mode value is asserted. The bridge must consume
the live guest `r4` directly.

## Switch implementation

Mirror exactly:

```text
stage = r3
mode  = r4
if stage >= GX_MAX_TEVSTAGE: return
GXSetTevOp((GXTevStageID)stage, (GXTevMode)mode)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement `GXSetTevColorIn`, `GXSetTevAlphaIn`, or any
neighboring TEV boundary.
