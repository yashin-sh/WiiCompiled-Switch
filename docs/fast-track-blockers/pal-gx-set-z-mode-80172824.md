# PAL GXSetZMode — 0x80172824

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x80172824
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000000
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_ALPHA_UPDATE
```

The distinct later target plus the stage written by the merged
GXSetAlphaUpdate bridge prove durable progression beyond PAL `0x801727F8`.

The current blocker record did not include `r4/r5`; do not infer them.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` implements:

```cpp
extern "C" void GX__SetZMode_80172824(uint32_t ce, uint32_t f, uint32_t ue) {
    GXSetZMode(
        static_cast<GXBool>(ce),
        static_cast<GXCompare>(f),
        static_cast<GXBool>(ue));
}
```

Therefore the live PPC ABI is:

```text
r3 -> compare enable
r4 -> GXCompare
r5 -> update enable
```

## Switch implementation

Read and forward the live `r3/r4/r5` values exactly. Do not hard-code or
fabricate the uncaptured `r4/r5` values.

The blocker diagnostic is extended to record `r4/r5` on subsequent runs.

Do not speculatively implement `GXSetZCompLoc` or any later pixel-state call.
