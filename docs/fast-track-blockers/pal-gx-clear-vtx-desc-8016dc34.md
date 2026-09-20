# PAL GXClearVtxDesc — 0x8016DC34

Tracking: #117, #162

## Hardware blocker

The first hardware run after #197 crosses `GXSetCurrentMtx` and stops at:

```text
kind   : DIRECT
target : 0x8016DC34
r1     : 0x80399008
r3     : 0x00000000
stage  : RMCP01_GX_SET_CURRENT_MTX
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__ClearVtxDesc_8016dc34`.

## Pinned semantics

The native override takes no PPC arguments. It:

- clears all 26 tracked `g_hleGxState.vtxDesc[]` entries to `GX_NONE`;
- invalidates the vertex-layout hash only if any descriptor actually changed;
- preserves vertex-array base/stride state;
- then calls Aurora `GXClearVtxDesc()`.

This is not equivalent to a bare no-op: the pinned HLE vertex decoder consumes
the tracked descriptor state when preparing later draws.

## Switch implementation

The rendered fast-track mirrors the pinned descriptor-state reset against the
same linked `g_hleGxState`, performs the conditional layout-hash invalidation,
and calls Aurora `GXClearVtxDesc()`.

Synthetic/headless builds retain the direct native boundary without importing
Aurora state into Nintendo-data-free public CI.

A dedicated `GXClearVtxDesc hits` counter is added to durable diagnostics.

No neighboring `GXSetVtxDesc`, `GXSetVtxAttrFmt`, indexed-array, or draw
boundary is implemented speculatively; those remain hardware-gated.
