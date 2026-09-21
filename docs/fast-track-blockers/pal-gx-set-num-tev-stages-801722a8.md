# PAL GXSetNumTevStages — 0x801722A8

Tracking: #117, #162

## Hardware blocker

The rendered run after merged #207 reaches:

```text
kind   : DIRECT
target : 0x801722a8
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_NUM_IND_STAGES
```

Because that stage is written inside the implemented GXSetNumIndStages bridge,
the later exact target is hardware proof that the previous boundary returned
and execution advanced.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetNumTevStages_801722a8(uint32_t n) {
    if (n > GX_MAX_TEVSTAGE) {
        return;
    }
    GXSetNumTevStages((u8)n);
}
```

The direct-call catalog passes only `cpu->gpr[3]`; hardware captured one.

## Switch implementation

Mirror exactly:

```text
count = r3
if count > GX_MAX_TEVSTAGE: return
GXSetNumTevStages((u8)count)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement `GXSetTevOrder`, `GXSetTevDirect`, or any neighboring
TEV boundary.
