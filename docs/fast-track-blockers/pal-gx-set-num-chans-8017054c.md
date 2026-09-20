# PAL GXSetNumChans — 0x8017054C

Tracking: #117, #162

## Hardware blocker

The first hardware run after #200 crosses `GXSetVtxAttrFmt` and stops at:

```text
kind   : DIRECT
target : 0x8017054C
r3     : 0x00000001
stage  : RMCP01_GX_SET_VTX_ATTR_FMT
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetNumChans_8017054c`.

## Pinned semantics

The override is intentionally minimal:

```cpp
extern "C" void GX__SetNumChans_8017054c(uint32_t n) {
    GXSetNumChans((u8)n);
}
```

The observed hardware value is `n = 1`.

## Switch implementation

The Switch bridge consumes `r3`, records stage `RMCP01_GX_SET_NUM_CHANS`,
narrows the value to `u8`, and forwards it directly to Aurora
`GXSetNumChans`.

Synthetic/headless builds retain the native-call seam without importing Aurora
into Nintendo-data-free public CI.

A dedicated `GXSetNumChans hits` counter is added to durable diagnostics.

No neighboring lighting/channel-control/texture/draw boundary is pre-ported.
