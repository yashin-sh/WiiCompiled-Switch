# Graphics backend notes

Aurora upstream currently couples its application layer to SDL3 and implements GX on top of WebGPU/Dawn, with desktop/mobile graphics backends including D3D12, Vulkan and Metal.

WiiCompiled-Switch already has a more specific seam than a generic source-port: at pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`, `GX_HLE_FIFO_Write8/16/32/Float` normally feed `HleFifoWrite`, and that runtime already decodes GX FIFO/state/draw traffic into Aurora GX calls. The current Switch fast-track intentionally replaces those helpers with a sink only so CPU/runtime bring-up can progress headlessly.

Therefore M3 should **reuse the pinned WiiCompiled FIFO decoder** rather than creating a second GX parser unless hardware proves that path unusable.

## Primary probe — Aurora GX + Dawn/WebGPU + Vulkan/Mesa/NVK

This is now the first route to test under #4/#162 because:

- it preserves the largest amount of pinned WiiCompiled/Aurora graphics work;
- `new-coke/strikers` demonstrates Aurora GX handling a complete Nintendo SDK game on supported desktop backends, including pipeline/shader warm-up and only a small explicit compatibility layer for missing GX calls;
- issue #4 contains an external real-Switch report claiming successful frame presentation through Aurora GX → Dawn/WebGPU → Vulkan/Mesa/NVK → libnx/NWindow.

The lower presentation half is now our own hardware validation: the isolated #162 probe has presented continuously changing full-screen colors on real Switch through `NWindow → VK_NN_vi_surface → loaderless NVK → VkSwapchainKHR → QueuePresentKHR`. The remaining external/unproven portion is Dawn/WebGPU + Aurora GX + WiiCompiled FIFO.

The probe should keep Aurora's **GX + graphics/pipeline** layers while bypassing desktop SDL application/input/audio services wherever Horizon-native services already exist.

Minimum progression:

1. ~~create/present a Horizon clear frame through the isolated loaderless NVK / `VK_NN_vi_surface` probe~~ — **hardware PASS**;
2. present a simple Vulkan triangle on the proven VI swapchain — **current frontier**;
3. place Dawn/WebGPU over the proven Vulkan/NVK path;
4. feed a fabricated Nintendo-data-free GX/FIFO sequence through pinned `HleFifoWrite` and obtain visible output;
5. measure CPU frame overhead, memory use and presentation stability on Tegra X1;
6. only then connect a private local RMCP01 stream.

The first direct-Vulkan probe has passed on hardware, so Dawn now has a known-good Horizon/NVK/VI presentation substrate to target. Any failure introduced by the next Dawn/Aurora stages can be attributed above the already-proven WSI/present layer.

## Fallback — Deko3D native Aurora backend

Deko3D remains the fallback if the Dawn/Vulkan/NVK path is incompatible, unstable, too memory-heavy or too expensive on the Switch CPU.

It is the established low-level devkitPro/libnx GPU API and likely offers the most control, but a direct Deko3D route requires substantially more Aurora integration work because Aurora's current GX implementation is WebGPU-oriented.

## Important constraints

- The #117 black-screen path is now hardware-classified as **active**: a 2026-09-18 run reached 126,563 dispatches and invoked `PostRetraceCallback` with guest retrace value 13,918. Keep the normal fast-track FIFO sink only as a stable baseline until isolated #162 proves the replacement graphics path.
- Do not import Aurora's full SDL application layer just to obtain GX rendering.
- Do not copy reconstructed game code from `new-coke/strikers`; use the project as an architecture/case-study reference. Upstream Aurora itself is MIT-licensed.
- Existing GX correctness issues #109–#112 remain independent prerequisites/guards around the shared decoder path.
- Pipeline compilation must be observable from the first-frame probe; `strikers` explicitly treats queued shader/pipeline work as a startup condition, so an empty frame must not automatically be diagnosed as guest/runtime failure.

Decision rule: test the path that reuses the most proven WiiCompiled/Aurora code first, then fall back to a native Deko3D backend only if real Switch measurements justify the extra renderer-porting cost.

See `STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md` and issue #162 for the concrete probe plan.
