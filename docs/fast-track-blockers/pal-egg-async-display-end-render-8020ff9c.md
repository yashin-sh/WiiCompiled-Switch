# PAL EGG::AsyncDisplay::endRender — 0x8020FF9C

Tracking: #117, #162

## Hardware blocker

```text
kind   : INDIRECT_CALL_MISS
target : 0x8020ff9c
r1     : 0x803990a8
r2     : 0x8038efa0
r3     : 0x8042944c
r4     : 0x00000000
r5     : 0x00000004
r13    : 0x8038cc00
stage  : RMCP01_FIFO_RENDER_WORK
```

This blocker is reached only after the rendered report emits
`PASS FIRST_RMCP01_FIFO_WORK`. Therefore `GXBegin` is hardware-crossed and
real RMCP01 vertex payload has produced Aurora work.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps this address to
`EGG::AsyncDisplay::endRender` HLE:

```text
preserve r3
lr = 0x8020FF9C
InvokeIndirectCpu(0x80219FB4)  # EGG::Display::copyEFBtoXFB
InvokeIndirectCpu(0x8016ED50)  # GXSetDrawDoneCallback
```

## Switch implementation

Resolve only this exact indirect native override. Route both nested calls
through the normal private translated dispatch table.

Do not manufacture `GXCopyDisp`, EFB copy completion, draw-done callback
success, or a present. Any missing nested target becomes the next real hardware
frontier.
