# Hardware result — M3 Dawn/WebGPU WGSL triangle + clean exit PASS (2026-09-18)

Tracking: #4, #162, #172, #174

Real Switch hardware validated the complete Dawn/WGSL triangle path, including clean teardown.

## Exact pins

```text
dawn-switch 77029ea85250c9bdddfc2f88034afb6b5356a031
mesa-switch b297e230ef88c6c88df2561becf864f979f494a6
```

## Observed hardware result

The durable SD report recorded:

```text
STAGE CREATE_SHADER PASS
STAGE CREATE_PIPELINE PASS
PASS FIRST_DAWN_TRIANGLE_PRESENT
ACTIVE frames=120
ACTIVE frames=240
ACTIVE frames=360
user requested exit
PASS LOOP frames=410
STAGE TEARDOWN surface-unconfigure PASS
STAGE TEARDOWN pipeline-release PASS
STAGE TEARDOWN queue-release PASS
STAGE TEARDOWN surface-release PASS
Device lost destroyed: Device was destroyed.
STAGE TEARDOWN device-release PASS
STAGE TEARDOWN adapter-release PASS
STAGE TEARDOWN instance-release PASS
STAGE TEARDOWN PASS
RESULT=PASS
```

The visible result was a large RGB-interpolated triangle over a changing-color background.

## What this proves

The following path is now hardware-proven end-to-end:

```text
WGSL
  -> Dawn shader module
  -> Dawn graphics pipeline
  -> WebGPU Draw(3)
  -> Dawn Vulkan backend
  -> loaderless mesa-switch / NVK
  -> VK_NN_vi_surface
  -> NWindow
  -> physical Switch display
```

It also proves the current explicit teardown sequence exits cleanly.

The `Device lost destroyed: Device was destroyed.` callback is expected during explicit device destruction and is not a failure in this run.

## Current frontier

The next graphics milestone is above Dawn itself:

```text
Aurora GX
  -> Dawn/WebGPU
  -> Vulkan/NVK
  -> Switch display
```

After Aurora GX is proven, the next step is the pinned WiiCompiled `HleFifoWrite` path with Nintendo-data-free synthetic GX/FIFO traffic.
