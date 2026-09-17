# Graphics backend notes

Aurora upstream currently couples its application layer to SDL3 and implements GX on top of WebGPU/Dawn, with desktop/mobile graphics backends including D3D12, Vulkan and Metal.

WiiCompiled-Switch already has a more specific seam than a generic source-port: at pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4`, `GX_HLE_FIFO_Write8/16/32/Float` normally feed `HleFifoWrite`, and that runtime already decodes GX FIFO/state/draw traffic into Aurora GX calls. The current Switch fast-track intentionally replaces those helpers with a sink only so CPU/runtime bring-up can progress headlessly.

Therefore M3 should **reuse the pinned WiiCompiled FIFO decoder** rather than creating a second GX parser unless hardware proves that path unusable.

## Primary probe — Aurora GX + Dawn/WebGPU + Vulkan/Mesa/NVK

This is now the first route to test under #4/#162 because:

- it preserves the largest amount of pinned WiiCompiled/Aurora graphics work;
- `new-coke/strikers` demonstrates Aurora GX handling a complete Nintendo SDK game on supported desktop backends, including pipeline/shader warm-up and only a small explicit compatibility layer for missing GX calls;
- issue #4 contains an external real-Switch report claiming successful frame presentation through Aurora GX → Dawn/WebGPU → Vulkan/Mesa/NVK → libnx/NWindow.

That external report is useful evidence, not yet our own validation. #162 exists to reproduce or falsify it on WiiCompiled-Switch hardware.

The probe should keep Aurora's **GX + graphics/pipeline** layers while bypassing desktop SDL application/input/audio services wherever Horizon-native services already exist.

Minimum progression:

1. create/present a Horizon clear frame;
2. present a simple triangle through Dawn/Vulkan/NVK;
3. feed a fabricated Nintendo-data-free GX/FIFO sequence through pinned `HleFifoWrite` and obtain visible output;
4. measure CPU frame overhead, memory use and presentation stability on Tegra X1;
5. only then connect a private local RMCP01 stream.

## Fallback — Deko3D native Aurora backend

Deko3D remains the fallback if the Dawn/Vulkan/NVK path is incompatible, unstable, too memory-heavy or too expensive on the Switch CPU.

It is the established low-level devkitPro/libnx GPU API and likely offers the most control, but a direct Deko3D route requires substantially more Aurora integration work because Aurora's current GX implementation is WebGPU-oriented.

## Important constraints

- Do not replace the active #117 fast-track FIFO sink until `fast-track-heartbeat-history.txt` classifies the current sustained black-screen run as active vs stalled.
- Do not import Aurora's full SDL application layer just to obtain GX rendering.
- Do not copy reconstructed game code from `new-coke/strikers`; use the project as an architecture/case-study reference. Upstream Aurora itself is MIT-licensed.
- Existing GX correctness issues #109–#112 remain independent prerequisites/guards around the shared decoder path.
- Pipeline compilation must be observable from the first-frame probe; `strikers` explicitly treats queued shader/pipeline work as a startup condition, so an empty frame must not automatically be diagnosed as guest/runtime failure.

Decision rule: test the path that reuses the most proven WiiCompiled/Aurora code first, then fall back to a native Deko3D backend only if real Switch measurements justify the extra renderer-porting cost.

See `STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md` and issue #162 for the concrete probe plan.
