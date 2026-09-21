# PAL GXSetAlphaUpdate — 0x801727F8

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x801727f8
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_COLOR_UPDATE
```

The later target plus the stage written by the merged GXSetColorUpdate bridge
prove durable progression beyond PAL `0x801727CC`.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetAlphaUpdate_801727f8(uint32_t en) {
    GXSetAlphaUpdate(static_cast<GXBool>(en));
}
```

Hardware captured `r3 = 1`.

## Switch implementation

Mirror exactly:

```text
enable = r3
GXSetAlphaUpdate((GXBool)enable)
```

Add the matching stage and hit counter for the next hardware proof. Do not
speculatively implement neighboring pixel-state boundaries.
