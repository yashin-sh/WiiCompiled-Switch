# M3 Dawn/WebGPU WGSL triangle probe

Tracking: #4, #162

This probe advances one layer above the hardware-validated Dawn clear/present result.

## Path under test

```text
WGSL vertex + fragment shader
        ↓
Dawn shader module
        ↓
Dawn render pipeline
        ↓
WebGPU render pass / Draw(3)
        ↓
Dawn Vulkan backend
        ↓
loaderless mesa-switch / NVK
        ↓
VK_NN_vi_surface
        ↓
NWindow / Switch display
```

The lower Dawn/Vulkan/NVK/presentation chain is already hardware-proven.

## Build

```sh
MKW_JOBS=4 bash scripts/build-m3-dawn-triangle-probe.sh
```

Output:

```text
m3-dawn-triangle-probe/WiiCompiled-Switch-m3-dawn-triangle-probe.nro
```

Copy to:

```text
/switch/WiiCompiled-Switch-m3-dawn-triangle-probe/
  WiiCompiled-Switch-m3-dawn-triangle-probe.nro
```

## Expected hardware result

A large RGB-interpolated triangle should be visible over a slowly changing dark background.

Press `+` to exit.

Durable report:

```text
/switch/WiiCompiled-Switch/m3-dawn-triangle-probe.txt
```

Expected PASS boundaries:

```text
STAGE CREATE_INSTANCE PASS
STAGE CREATE_SURFACE PASS
STAGE REQUEST_ADAPTER PASS
STAGE REQUEST_DEVICE PASS
STAGE CONFIGURE_SURFACE PASS
STAGE CREATE_SHADER PASS
STAGE CREATE_PIPELINE PASS
PASS FIRST_DAWN_TRIANGLE_PRESENT
...
RESULT=PASS
```

A PASS proves WGSL -> Dawn pipeline -> Vulkan/NVK rasterization/presentation on real Switch. It still does not prove Aurora GX or WiiCompiled FIFO traffic.
