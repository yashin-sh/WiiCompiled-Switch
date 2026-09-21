# PAL GXSetColorUpdate — 0x801727CC

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x801727cc
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_BLEND_MODE
```

The later target plus the stage written by the merged GXSetBlendMode bridge
prove durable progression beyond PAL `0x8017277C`.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetColorUpdate_801727cc(uint32_t en) {
    GXSetColorUpdate(static_cast<GXBool>(en));
}
```

Hardware captured `r3 = 1`.

## Switch implementation

Mirror exactly:

```text
enable = r3
GXSetColorUpdate((GXBool)enable)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement the neighboring pixel-state boundaries.
