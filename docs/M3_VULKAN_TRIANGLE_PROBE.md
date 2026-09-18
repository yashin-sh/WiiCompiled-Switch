# M3 Vulkan triangle probe

Tracking: #4, #162

## Purpose

This is the second isolated graphics proof after the real-Switch NVK/VI clear-frame PASS.

The probe keeps the same hardware-proven presentation substrate:

```text
libnx / Horizon NWindow
        ↓
VK_NN_vi_surface
        ↓
mesa-switch loaderless NVK
        ↓
VkSwapchainKHR
```

and adds the first real graphics pipeline:

```text
GLSL vertex + fragment shaders
        ↓
SPIR-V
        ↓
VkRenderPass / VkPipeline
        ↓
vkCmdDraw(3)
        ↓
QueuePresentKHR
```

No Mario Kart Wii code, assets or Nintendo-derived fixtures are involved.

## Independent implementation

The triangle probe is implemented independently in this repository. External `switch-nvk` work was used only as evidence that NVK triangle rasterisation has been demonstrated on Switch hardware; its GPL-2.0 code/shaders are not copied into this GPL-3.0 WiiCompiled-derived repository.

## Build

```sh
git checkout main
git pull
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-m3-vulkan-triangle-probe.sh
```

The shared M3 Docker image now includes `glslangValidator`. The two small first-party GLSL shaders are compiled to SPIR-V during the local build, then converted into generated C++ headers that stay ignored/uncommitted.

Output:

```text
m3-triangle-probe/WiiCompiled-Switch-m3-vulkan-triangle-probe.nro
```

## Hardware PASS criterion

Copy to:

```text
/switch/WiiCompiled-Switch-m3-vulkan-triangle-probe/
  WiiCompiled-Switch-m3-vulkan-triangle-probe.nro
```

Launch through hbmenu title override / application mode.

Expected PASS:

- dark full-screen background;
- one large triangle with red/green/blue vertex interpolation;
- stable continuous presentation;
- `+` exits cleanly.

Diagnostic report:

```text
/switch/WiiCompiled-Switch/m3-vulkan-triangle-probe.txt
```

A PASS validates shader compilation, shader modules, render pass, graphics-pipeline creation, rasterisation and presentation on the already-proven NVK/VI surface.

## Hardware result — 2026-09-18

Real Switch hardware displayed the triangle successfully. This validates the direct Vulkan/NVK graphics-pipeline path through shader modules, render pass, graphics pipeline, rasterisation, swapchain and physical display output.

The first successful rendering run did **not** create `m3-vulkan-triangle-probe.txt`. That was isolated to report initialization: libnx normally mounts `sdmc:` before NRO `main()`, while the probe incorrectly gated `fopen()` on a second `fsdevMountSdmc()` call. The renderer itself was already working. The probe now prefers the existing runtime mount and only mounts/unmounts `sdmc:` itself as a fallback.

Native Vulkan triangle frame ✅

This is still **not** an Aurora GX frame, WiiCompiled FIFO frame, or RMCP01/Mario Kart frame.

## After PASS

The next #162 step is Dawn/WebGPU over this proven Vulkan/NVK graphics path. Only after Dawn works do we add Aurora GX and fabricated Nintendo-data-free input through pinned WiiCompiled `HleFifoWrite`.
