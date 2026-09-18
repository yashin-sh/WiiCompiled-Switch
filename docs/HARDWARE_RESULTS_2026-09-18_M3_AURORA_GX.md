# Hardware result — M3 Aurora GX triangle — 2026-09-18

Tracking: #4, #162, #176

## Result

**PASS on real Nintendo Switch hardware.**

The isolated Nintendo-data-free probe presented a visible Aurora GX triangle through:

```text
GX API
  ↓
Aurora FIFO / GX
  ↓
Aurora shader + graphics pipeline
  ↓
Dawn / WebGPU
  ↓
Vulkan / loaderless NVK
  ↓
VK_NN_vi_surface / NWindow
  ↓
physical Switch display
```

Pinned revisions:

- WiiCompiled: `a135beb201042b20f390c6695ca6b26768820fb4`
- dawn-switch: `77029ea85250c9bdddfc2f88034afb6b5356a031`
- mesa-switch: `b297e230ef88c6c88df2561becf864f979f494a6`

## Hardware evidence

The durable SD report recorded:

```text
STAGE NWINDOW PASS
STAGE CREATE_INSTANCE PASS
STAGE CREATE_SURFACE PASS
STAGE REQUEST_ADAPTER PASS
STAGE REQUEST_DEVICE PASS
STAGE CONFIGURE_SURFACE PASS format=22 presentMode=1
STAGE AURORA_GFX_INIT PASS
STAGE GX_INIT PASS
PASS FIRST_AURORA_GX_TRIANGLE_PRESENT
ACTIVE frames=120
ACTIVE frames=240
ACTIVE frames=360
ACTIVE frames=480
user requested exit
PASS LOOP frames=563
STAGE TEARDOWN aurora-gfx PASS
STAGE TEARDOWN surface-unconfigure PASS
STAGE TEARDOWN PASS
RESULT=PASS
```

The Dawn device-lost callback after surface/device teardown reported `reason=2: Device was destroyed.`; this occurred during the intentional destruction sequence and did not turn the run into a failure.

## Init-only warnings

During `GXInit`, Aurora logged several:

```text
Dropping command SetViewport
Dropping command SetScissor
```

These warnings happened before the first active render frame. They did not prevent the GX triangle from being presented or the 563-frame loop from remaining active. They remain useful diagnostics if viewport/scissor publication becomes relevant in later game-facing probes.

## What this proves

This closes the **Aurora GX frame** milestone:

- Aurora GX/GFX from the pinned WiiCompiled tree can initialize on Horizon;
- GX state can be configured;
- a triangle issued through GX calls reaches Aurora's FIFO/command processor;
- Aurora's GX shader/pipeline reaches the hardware-proven Dawn/Vulkan/NVK path;
- presentation remains active for hundreds of frames;
- explicit teardown exits cleanly.

## What this does not prove

This probe bypasses the pinned WiiCompiled runtime `HleFifoWrite` decoder when generating the triangle.

The next gate is therefore:

```text
fabricated Nintendo-data-free GX FIFO bytes
                ↓
pinned WiiCompiled HleFifoWrite
                ↓
Aurora GX
                ↓
Dawn / Vulkan / NVK
                ↓
Switch display
```

Only after that PASS should the project connect a private local RMCP01 graphics stream.
