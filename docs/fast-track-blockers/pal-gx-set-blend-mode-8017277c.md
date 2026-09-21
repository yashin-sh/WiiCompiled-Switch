# PAL GXSetBlendMode — 0x8017277C

Tracking: #117, #162

## Hardware blocker

The rendered run after merged #210 reaches:

```text
kind   : DIRECT
target : 0x8017277c
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_TEV_ORDER
```

Because that stage is written inside the implemented GXSetTevOrder bridge, the
later exact target is hardware proof that the previous boundary returned and
execution advanced.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetBlendMode_8017277c(
    uint32_t t, uint32_t s, uint32_t d, uint32_t op) {
    GXSetBlendMode(
        static_cast<GXBlendMode>(t),
        static_cast<GXBlendFactor>(s),
        static_cast<GXBlendFactor>(d),
        static_cast<GXLogicOp>(op));
}
```

The blocker captured only `r3 = 0`; its diagnostic format does not include
`r4-r6`. Therefore no hardware blend-factor or logic-op values are asserted.
The bridge must consume the live guest registers directly.

## Switch implementation

Mirror exactly:

```text
type      = r3
srcFactor = r4
dstFactor = r5
logicOp   = r6
GXSetBlendMode(
    (GXBlendMode)type,
    (GXBlendFactor)srcFactor,
    (GXBlendFactor)dstFactor,
    (GXLogicOp)logicOp)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement neighboring pixel-state boundaries.
