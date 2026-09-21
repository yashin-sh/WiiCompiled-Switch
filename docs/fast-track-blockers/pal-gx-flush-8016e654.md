# PAL GXFlush — 0x8016E654

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x8016e654
r1     : 0x80399078
r2     : 0x8038efa0
r3     : 0x90087ee0
r4     : 0x00000001
r5     : 0x00000001
r6     : 0x80245816
r13    : 0x8038cc00
stage  : RMCP01_GX_PRESENTED
```

The stage is only published after the rendered `GXCopyDisp` bridge completes
a successful `g_surface.Present()`. The renderer separately records:

```text
PASS FIRST_RMCP01_GX_PRESENT hadWork=1
```

This is durable proof that the previous `GXSetCopyFilter` frontier was
crossed and that the first game-facing RMCP01 GPU present succeeded.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps `0x8016E654` to:

```cpp
GXFlush();
```

No PPC arguments are consumed.

## Switch implementation

Set `RMCP01_GX_FLUSH` and call Aurora `GXFlush()` only in the rendered
target. Preserve a no-op host side effect for headless/synthetic probes.

Do not pre-port the following GX/resource boundary.
