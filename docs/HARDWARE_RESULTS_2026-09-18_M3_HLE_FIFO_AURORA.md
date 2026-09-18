# Hardware result — M3 pinned HleFifoWrite → Aurora GX — 2026-09-18

Tracking: #4, #162, #178, #179

## Result

**PASS on real Nintendo Switch hardware.**

A fabricated Nintendo-data-free GX stream was fed byte-by-byte through the exact
pinned WiiCompiled `HleFifoWrite(value, 1)` implementation and reached the
hardware-proven Aurora GX → Dawn/WebGPU → Vulkan/NVK → NWindow renderer.

Pins:

- WiiCompiled: `a135beb201042b20f390c6695ca6b26768820fb4`
- dawn-switch: `77029ea85250c9bdddfc2f88034afb6b5356a031`
- mesa-switch: `b297e230ef88c6c88df2561becf864f979f494a6`

## Hardware evidence

The durable report recorded:

```text
STAGE AURORA_GFX_INIT PASS
STAGE GX_FIXED_STATE PASS
STAGE HLE_FIFO_DECODER_RESET PASS
STAGE HLE_FIFO_STREAM begin bytes=69 mode=bytewise
STAGE HLE_FIFO_STREAM PASS path=raw-direct
  posDesc=1 clr0Desc=1
  posCnt=1 posType=4 posFrac=0
  clrCnt=1 clrType=5
  vertsRemaining=3 fifoBytes=0 inBegin=0 hadWork=1
PASS FIRST_HLE_FIFO_AURORA_TRIANGLE_PRESENT
ACTIVE frames=120
...
ACTIVE frames=1320
user requested exit
PASS LOOP frames=1435
STAGE TEARDOWN aurora-gfx PASS
STAGE TEARDOWN surface-unconfigure PASS
STAGE TEARDOWN PASS
RESULT=PASS
```

The triangle remained active for 1,435 frames before the user-requested exit.

## Raw-direct post-state

The first hardware run exposed a probe-validation error rather than a decoder
failure. The pinned all-direct fast path submits the complete packet and
intentionally leaves `vertsRemaining` at the original vertex count. For this
three-vertex packet the correct post-state is therefore `vertsRemaining=3`,
with `inBegin=0`, an empty FIFO and `hadWork=1`.

#179 corrected the validator without changing WiiCompiled HLE semantics.

## What this proves

The following complete synthetic path is hardware-proven:

```text
GX command bytes
  ↓
pinned WiiCompiled HleFifoWrite
  ↓
CP VCD/VAT + raw-direct draw decode
  ↓
Aurora GX
  ↓
Dawn/WebGPU
  ↓
Vulkan / loaderless NVK
  ↓
physical Switch display
```

This closes the last Nintendo-data-free graphics gate before a local RMCP01
renderer integration.

## Next gate

The next build keeps the stable #117 headless fast-track unchanged and adds a
separate local-only rendered variant:

```text
local RMCP01 translated product
  ↓
GX_HLE_FIFO_Write*
  ↓
pinned HleFifoWrite
  ↓
Aurora GX
  ↓
Dawn / NVK
  ↓
GXCopyDisp
  ↓
NWindow / Switch display
```

A first successful present from that variant is the **RMCP01 graphics frame**
milestone, not another synthetic probe.
