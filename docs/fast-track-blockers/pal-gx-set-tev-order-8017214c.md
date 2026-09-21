# PAL GXSetTevOrder — 0x8017214C

Tracking: #117, #162

## Hardware blocker

The rendered run after merged #209 reaches:

```text
kind   : DIRECT
target : 0x8017214c
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_TEV_OP
```

Because that stage is written inside the implemented GXSetTevOp bridge, the
later exact target is hardware proof that the previous boundary returned and
execution advanced.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetTevOrder_8017214c(
    uint32_t s, uint32_t c, uint32_t m, uint32_t col) {
    if (!TevStageOk(s)) return;
    GXSetTevOrder(
        (GXTevStageID)s,
        (GXTexCoordID)c,
        (GXTexMapID)m,
        (GXChannelID)col);
}
```

`TevStageOk(s)` requires `s < GX_MAX_TEVSTAGE`.

The blocker captured only `r3 = 0`; its diagnostic format does not include
`r4-r6`. Therefore no hardware texcoord, texmap, or channel values are
asserted. The bridge must consume the live guest registers directly.

## Switch implementation

Mirror exactly:

```text
stage    = r3
texCoord = r4
texMap   = r5
channel  = r6
if stage >= GX_MAX_TEVSTAGE: return
GXSetTevOrder(
    (GXTevStageID)stage,
    (GXTexCoordID)texCoord,
    (GXTexMapID)texMap,
    (GXChannelID)channel)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement neighboring TEV boundaries.
