# PAL GXSetCullMode — 0x8016F3B8

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x8016f3b8
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000002
r4     : 0x00000000
r5     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_Z_MODE
```

The distinct later target plus the stage written by the merged GXSetZMode
bridge prove durable progression beyond PAL `0x80172824`.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetCullMode_8016f3b8(uint32_t m) {
    GXSetCullMode(static_cast<GXCullMode>(m));
}
```

Hardware captured `r3 = 2`.

## Switch implementation

Mirror exactly:

```text
mode = r3
GXSetCullMode((GXCullMode)mode)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement `GXSetCoPlanar`, `GXSetClipMode`, or later state
boundaries.
