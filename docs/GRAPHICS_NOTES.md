# Graphics backend notes

The Aurora tree bundled with pinned WiiCompiled couples its application layer to SDL3 and implements GX on top of WebGPU/Dawn. The Switch integration keeps the GX/graphics layers and supplies a Horizon platform seam.

WiiCompiled-Switch already has a more specific seam than a generic source-port: at pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`, `GX_HLE_FIFO_Write8/16/32/Float` normally feed `HleFifoWrite`, and that runtime already decodes GX FIFO/state/draw traffic into Aurora GX calls. The headless control target replaces those helpers with a sink; the separate rendered target keeps the decoder/native Aurora path.

Therefore M3 should **reuse the pinned WiiCompiled FIFO decoder** rather than creating a second GX parser unless hardware proves that path unusable.

## Primary probe — Aurora GX + Dawn/WebGPU + Vulkan/Mesa/NVK

This is the hardware-proven primary renderer route under #4/#162:

- it preserves the largest amount of pinned WiiCompiled/Aurora graphics work;
- `new-coke/strikers` demonstrates Aurora GX handling a complete Nintendo SDK game on supported desktop backends, including pipeline/shader warm-up and only a small explicit compatibility layer for missing GX calls;
- issue #4 contains an external real-Switch report claiming successful frame presentation through Aurora GX → Dawn/WebGPU → Vulkan/Mesa/NVK → libnx/NWindow.

The complete Nintendo-data-free renderer path is now our own hardware validation: Dawn/WebGPU, Aurora GX and the exact pinned WiiCompiled `HleFifoWrite` decoder have each presented successfully on real Switch through loaderless NVK and `NWindow`. The separate rendered target has also decoded real local RMCP01 traffic and repeatedly presented frames. Recognizable Mario Kart Wii pixels and full game correctness remain unproven.

The probe should keep Aurora's **GX + graphics/pipeline** layers while bypassing desktop SDL application/input/audio services wherever Horizon-native services already exist.

Minimum progression:

1. ~~create/present a Horizon clear frame through the isolated loaderless NVK / `VK_NN_vi_surface` probe~~ — **hardware PASS**;
2. ~~present a simple Vulkan triangle on the proven VI swapchain~~ — **hardware PASS: visible triangle on real Switch**; the first run exposed only an SD-report initialization bug, not a rendering failure;
3. ~~place Dawn/WebGPU over the proven Vulkan/NVK path~~ — **hardware PASS: 1,507-frame real-Switch present loop**;
4. ~~present a WGSL triangle through a Dawn graphics pipeline~~ — **hardware PASS: visible RGB triangle and clean exit**;
5. ~~present a Nintendo-data-free triangle through the actual Aurora GX API/FIFO/command processor~~ — **hardware PASS: visible triangle, 563-frame loop, clean teardown**;
6. ~~feed a fabricated Nintendo-data-free GX/FIFO sequence through pinned `HleFifoWrite` and obtain visible output~~ — **hardware PASS: raw-direct path, 1,435-frame stable loop, clean teardown**;
7. connect a private local RMCP01 stream through the same renderer — **hardware PASS for real FIFO work and repeated GPU presents; recognizable game pixels pending**;
8. measure CPU frame overhead, memory use and presentation stability on Tegra X1.

The direct-Vulkan clear/triangle probes, Dawn clear/present, Dawn WGSL triangle, Aurora GX triangle and pinned `HleFifoWrite` synthetic FIFO path have now all passed on hardware. The FIFO run remained active for 1,435 frames and exited cleanly. That graphics chain is established for local RMCP01 FIFO work and GPU presents.
The latest accepted Discovery frontier is Scale `0x80171180`, after ten
texture-matrix returns. The coordinate NRO has been transferred but its new
reports and screen observation are pending. See
[the accepted result](HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_TEX_COORD_SCALE_FRONTIER.md)
and [the candidate record](GX_TEX_COORD_BATCH_2026-10-03.md).

## Fallback — Deko3D native Aurora backend

Deko3D remains the fallback if the Dawn/Vulkan/NVK path is incompatible, unstable, too memory-heavy or too expensive on the Switch CPU.

It is the established low-level devkitPro/libnx GPU API and likely offers the most control, but a direct Deko3D route requires substantially more Aurora integration work because Aurora's current GX implementation is WebGPU-oriented.

## Important constraints

- The #117 black-screen path is hardware-classified as **active**: a 2026-09-18 run reached 126,563 dispatches and invoked `PostRetraceCallback` with guest retrace value 13,918. Keep that headless FIFO sink as the control baseline while each new rendered boundary is validated.
- Do not import Aurora's full SDL application layer just to obtain GX rendering.
- Do not copy reconstructed game code from `new-coke/strikers`; use the project as an architecture/case-study reference. Upstream Aurora itself is MIT-licensed.
- Existing GX correctness issues #109–#112 remain independent prerequisites/guards around the shared decoder path.
- Pipeline compilation must be observable from the first-frame probe; `strikers` explicitly treats queued shader/pipeline work as a startup condition, so an empty frame must not automatically be diagnosed as guest/runtime failure.

Decision rule: test the path that reuses the most proven WiiCompiled/Aurora code first, then fall back to a native Deko3D backend only if real Switch measurements justify the extra renderer-porting cost.

See `STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md` and issue #162 for the concrete probe plan.
