# Hardware result — M3 Dawn/WebGPU clear presentation PASS (2026-09-18)

Tracking: #4, #162, #171

Real Switch hardware validated the isolated Dawn/WebGPU clear probe merged in #171.

## Exact pins

```text
dawn-switch 77029ea85250c9bdddfc2f88034afb6b5356a031
mesa-switch b297e230ef88c6c88df2561becf864f979f494a6
```

## Observed hardware result

The durable SD report recorded:

```text
STAGE NWINDOW PASS
STAGE CREATE_INSTANCE PASS
STAGE CREATE_SURFACE PASS
STAGE REQUEST_ADAPTER PASS
STAGE REQUEST_DEVICE PASS
STAGE CONFIGURE_SURFACE PASS format=22 presentMode=1
PASS FIRST_DAWN_PRESENT
ACTIVE frames=120
...
ACTIVE frames=1440
user requested exit
PASS LOOP frames=1507
RESULT=PASS
```

The surface was configured at 1280x720 and exposed two formats, two present modes and two alpha modes.

## What this proves

```text
WebGPU API
  -> Dawn
  -> Dawn Vulkan backend
  -> loaderless mesa-switch / NVK
  -> VK_NN_vi_surface
  -> NWindow
  -> physical Switch display
```

This closes the Dawn clear/presentation risk above the previously-proven direct Vulkan/NVK layer.

The next isolated gate is a Dawn/WGSL triangle using the same hardware-proven surface and NVK path.
