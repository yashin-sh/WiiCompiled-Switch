# PAL GXSetVtxDesc — 0x8016D3A4

Tracking: #117, #162

## Hardware blocker

The first hardware run after #198 crosses `GXClearVtxDesc` and stops at:

```text
kind   : DIRECT
target : 0x8016D3A4
r1     : 0x80399008
r3     : 0x00000009
stage  : RMCP01_GX_CLEAR_VTX_DESC
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetVtxDesc_8016d3a4`.

The observed attribute value `9` is `GX_VA_POS`. The blocker diagnostic does
not log `r4`, so the type is not guessed.

## Pinned semantics

The native override consumes `r3 = attr` and `r4 = type`.

It:

- canonicalizes NBT to NRM for `g_hleGxState.vtxDesc`;
- rejects out-of-range / null attributes;
- updates the tracked descriptor;
- keeps the separate NBT slot cleared after canonicalization;
- invalidates the cached vertex-layout hash only when the tracked type changes;
- returns without an Aurora call for matrix-index attributes;
- forwards normal attributes to Aurora;
- converts `GX_INDEX8` and `GX_INDEX16` to `GX_DIRECT` for Aurora because
  the pinned immediate decoder expands indexed attributes into its packed direct
  stream.

## Switch implementation

The Switch bridge mirrors those pinned state and forwarding rules exactly and
adds stage `RMCP01_GX_SET_VTX_DESC`.

Synthetic/headless builds retain the native direct-call boundary without
pulling Aurora into Nintendo-data-free public CI.

A dedicated `GXSetVtxDesc hits` counter is added to durable diagnostics.

No `GXSetVtxAttrFmt`, array, texcoord, begin/draw, or later vertex boundary is
pre-ported; each remains hardware-gated.
