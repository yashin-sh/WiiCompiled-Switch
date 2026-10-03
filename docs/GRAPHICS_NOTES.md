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
The earlier coordinate Discovery run crosses all eight
`Gen2(c,1,4,60,0,125)`, `Scale(c,0,0,0)` and `Bias(c,0,0)` triples for c=0..7,
then reaches GXSetTevDirect `0x80171B58`, stage 0, without return. Enabled
Scale/Bias branches remain host-tested only, and other TEV neighbors remain
static. The user confirmed black output. The preceding snapshot at dispatch
605367 records 1556 FIFO writes and 99 successful presents / 0 failures;
it does not measure later native emissions or prove recognizable pixels. See
[the accepted result](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md)
and [the coordinate record](GX_TEX_COORD_BATCH_2026-10-03.md).
The subsequent [audit hardware run](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md),
`b3484117` / `7ecbc8a9...`, preserves that normal path and reaches the same
Direct stage-0 frontier at dispatch 608381. Its preceding snapshot remains
1556 FIFO writes and 99 successful presents / 0 failures; the user again
saw black. Negative failure branches remain host/static evidence. The
[six-setter TEV batch](GX_TEV_SCALAR_BATCH_2026-10-03.md) subsequently passed
all five GitHub workflows and hardware-crossed its 96 default-tuple calls
across stages 0..15. The [latest result](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md) stops at
KColor ID 0, pointer `0x80398FCC`, dispatch 605056 / 98.265 seconds. The
user saw black and an error at exit. Its changed snapshot 604804 precedes
the loop, retaining 1556 FIFO writes and 99 successful presents / 0 failures;
it does not measure later native emissions or prove pixels. Alternate TEV
inputs retain host contracts, and KColor was that scalar run's arrival boundary.
The [TEV color/table batch](GX_TEV_COLOR_BATCH_2026-10-03.md) passed all five
GitHub workflows and its exact private build (code `1333b0e2`, NRO `a56be881...`).
Its [fresh console result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
now establishes all twelve calls returned, with AlphaCompare `0x80172088`
as that run's arrival boundary. The separate
[AlphaCompare candidate](GX_ALPHA_COMPARE_2026-10-03.md) preserves the pinned
native forwarding and existing host validity flag. Its local contracts, all
five exact-code GitHub workflows and private Rendered Discovery build pass;
its subsequent [console run](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
accepts AlphaCompare returned on (7,0,0,7,0), through existing ZMode to Fog.
Fog type 0, four f64 parameters and readable RGBA 255,255,255,255 are captured.
That preceding run stopped before Fog returned; the user confirmed black output
and an error. The [later Fog/ZCompLoc result](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
now accepts both new bridge returns and the existing pixel setup. Native init
of a 4×4 depth texture passed; GXInitTexObjLOD rejects its valid format 22
because the structural layout table lacks it. The current visual observation
is pending, and recognizable game pixels remain unproven.

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


## Earlier console result — TEV colors crossed (2026-10-03)

The [fresh color/table hardware result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
supersedes the earlier pending color/table status. All twelve executed calls
returned through the coherent later caller; AlphaCompare `0x80172088`,
(7,0,0,7,0), is the new DIRECT hard stop. Black output and a crash persist.
Actual RGBA bytes and recognizable game pixels remain unproven. The elapsed
time includes an unexplained watchdog sampling gap, so it is not a performance
measurement. Prior dated results above retain their original scope.


## Earlier console result — AlphaCompare crossed (2026-10-03)

The [fresh AlphaCompare hardware result](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md)
establishes its observed tuple returned. Fog `0x801722CC` is the new DIRECT
hard stop at dispatch 603961 / 99.156 seconds, with actual float parameter bits
and readable color captured. All 96 watchdog samples are ACTIVE. Preceding
present counters do not prove visible pixels. The user confirms a black screen
followed by an error; the exact on-screen wording is unavailable. Earlier dated
sections retain their original scope.


## Latest console result — Fog/ZCompLoc crossed (2026-10-03)

The [fresh hardware result](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md)
establishes the admitted Fog call, ZCompLoc(1) and existing pixel setup returned.
The next stop is GXInitTexObjLOD at `0x80170A4C`, dispatch 609384 / 109.572
seconds. Native init passed for the 4×4 `GX_TF_Z24X8` object; the LOD layout
validator lacks full format 22. Its forwarding and later drawing remain
unproven. The later snapshot records 1558 guest FIFO writes and the same 99
successful presents; those counters do not establish a visible frame. Current
screen observation is pending. Earlier dated sections retain their scope.
