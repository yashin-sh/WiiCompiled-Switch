# Hardware result — M3 Vulkan triangle

Date: 2026-09-18  
Tracking: #4, #162

## Result

**PASS — visible Vulkan triangle on real Nintendo Switch hardware.**

The isolated M3 triangle NRO displayed the expected triangle on the physical Switch display using the already-proven NVK/VI presentation substrate.

Validated path:

```text
first-party GLSL
        ↓
SPIR-V
        ↓
VkShaderModule
        ↓
VkRenderPass / VkGraphicsPipeline
        ↓
vkCmdDraw(3)
        ↓
VkSwapchainKHR
        ↓
loaderless mesa-switch / NVK
        ↓
VK_NN_vi_surface / NWindow
        ↓
physical Switch display
```

This advances the graphics proof from a native clear/present to actual shader execution, pipeline creation and rasterisation.

## Diagnostic-file caveat

The successful rendering run did not produce:

```text
/switch/WiiCompiled-Switch/m3-vulkan-triangle-probe.txt
```

This is a diagnostics bug, not evidence of a renderer failure.

libnx runtime startup normally mounts `sdmc:` before NRO `main()`. The probe then attempted `fsdevMountSdmc()` again and only opened the report if that second mount returned success. Therefore an already-available SD filesystem could leave `g_report` null while Vulkan continued normally.

The probe has been hardened to:

1. use the already-mounted `sdmc:` first;
2. create/open the report directly;
3. call `fsdevMountSdmc()` only as a fallback;
4. unmount only if the probe itself created the fallback mount.

The same narrow fix is applied to the clear-frame probe because it used the same logger pattern.

## What this proves

Hardware-valid:

- Vulkan shader modules;
- render pass / framebuffer setup;
- graphics-pipeline creation;
- triangle rasterisation;
- NVK GPU submission;
- swapchain presentation;
- physical Switch display output.

Not yet proven by this result:

- Dawn/WebGPU;
- Aurora GX;
- pinned WiiCompiled `HleFifoWrite` on Switch;
- fabricated GX FIFO rendering;
- local RMCP01 GX traffic;
- first Mario Kart Wii frame.

## Next frontier

```text
Vulkan triangle ✅
        ↓
Dawn / WebGPU
        ↓
Aurora GX
        ↓
pinned WiiCompiled HleFifoWrite
        ↓
Nintendo-data-free synthetic FIFO
        ↓
local RMCP01 GX stream
        ↓
first Mario Kart Wii frame
```
