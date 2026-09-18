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

## Durable hardware log

The follow-up run after the SD-report fix produced the expected persistent report and confirms the full triangle path:

- `vkCreateInstance -> 0`;
- `vkCreateViSurfaceNN -> 0`;
- GPU: `NVIDIA Tegra X1 (GM20B) (NVK GM20B)`, Vulkan API `1.3.354`, queue family `0`;
- `VK_KHR_swapchain`: present;
- swapchain creation: success, `1280x720`, 3 requested/3 returned images, format `37`, present mode `2`;
- render pass: success;
- vertex and fragment shader modules: success;
- pipeline layout: success;
- graphics pipeline: success;
- `PASS FIRST_TRIANGLE_PRESENT`;
- sustained `ACTIVE` heartbeats through frame `5880`;
- user exit with `+`;
- final `PASS LOOP frames=5888` and `RESULT=PASS`.

This also hardware-validates the mount-safe report fix from #168: the durable `.txt` now exists and survived the successful run.

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
