# PAL GXSetChanMatColor — 0x80170474

Tracking: #117, #162

## Hardware blocker

The first hardware run after #201 crosses `GXSetNumChans` and stops at:

```text
kind   : DIRECT
target : 0x80170474
r1     : 0x80399008
r3     : 0x00000004
stage  : RMCP01_GX_SET_NUM_CHANS
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this exact PAL entry point
to `GX__SetChanMatColor_80170474`.

## Pinned semantics

The native override consumes:

```text
channel  = r3
colorPtr = r4
```

It ensures an Aurora frame is active, performs `Memory::Read32(colorPtr)`,
decodes the packed word as RGBA bytes, and calls:

```cpp
GXSetChanMatColor((GXChannelID)channel, color);
```

The observed hardware channel value is `4`. The concrete `r4` value is not
guessed because the durable blocker currently records only `r3`.

## Switch implementation

The Switch bridge mirrors the pinned semantics directly:

- reads channel from PPC `r3`;
- reads guest color pointer from PPC `r4`;
- calls `EnsureAuroraFrameActive()`;
- reads the guest color with `Memory::Read32`;
- decodes through pinned `DecodeGxColor`;
- forwards to Aurora `GXSetChanMatColor`.

Synthetic/headless builds retain the direct-call seam without importing Aurora
or guest memory into Nintendo-data-free public CI.

A dedicated `GXSetChanMatColor hits` counter is added to durable diagnostics.

No neighboring ambient-color, channel-control, light-object, texture, or draw
boundary is pre-ported.
