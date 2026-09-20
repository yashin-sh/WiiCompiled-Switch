# PAL GXSetVtxAttrFmt — 0x8016DC68

Tracking: #117, #162

## Hardware blocker

The first hardware run after #199 crosses `GXSetVtxDesc` and stops at:

```text
kind   : DIRECT
target : 0x8016DC68
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_VTX_DESC
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetVtxAttrFmt_8016dc68`.

The observed `r3 = 0` is `GX_VTXFMT0`; `r4..r7` are intentionally not
guessed because the blocker diagnostic does not record them.

## Pinned semantics

The native override consumes:

```text
vtxfmt = r3
attr   = r4
cnt    = r5
type   = r6
frac   = r7
```

It:

- canonicalizes `GX_VA_NBT` to `GX_VA_NRM` for tracked HLE format state;
- updates the tracked count/type/fraction for valid internal vtxfmt/attr ranges;
- clears the separate NBT slot after NBT canonicalization;
- invalidates the cached vertex-layout hash only when the tracked format changes;
- returns without Aurora for invalid public vtxfmt/attribute ranges;
- otherwise forwards the original public attribute and format tuple to Aurora
  `GXSetVtxAttrFmt`.

## Switch implementation

The Switch bridge mirrors those pinned state and forwarding rules and records
stage `RMCP01_GX_SET_VTX_ATTR_FMT`.

Synthetic/headless builds retain the direct-call seam without importing Aurora
state into Nintendo-data-free public CI.

A dedicated `GXSetVtxAttrFmt hits` counter is added to durable diagnostics.

No array, texcoord, begin/draw, or later vertex boundary is pre-ported.
