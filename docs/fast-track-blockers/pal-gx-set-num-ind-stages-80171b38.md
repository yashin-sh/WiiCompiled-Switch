# PAL GXSetNumIndStages — 0x80171B38

Tracking: #117, #162

## Hardware blocker

The rendered run after merged #206 reaches:

```text
kind   : DIRECT
target : 0x80171b38
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_NUM_TEX_GENS
```

Because that stage is written inside the implemented GXSetNumTexGens bridge,
the later exact target is hardware proof that the previous boundary returned
and execution advanced.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetNumIndStages_80171b38(uint32_t n) {
    GXSetNumIndStages((u8)n);
}
```

The direct-call catalog passes only `cpu->gpr[3]`; hardware captured zero.

## Switch implementation

Mirror exactly:

```text
count = r3
GXSetNumIndStages((u8)count)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement `GXSetTevDirect` or any neighboring indirect/TEV
boundary.
