# M3 Dawn / NVK offscreen probe

Tracking: #4, #162

## Purpose

This is the first Dawn/WebGPU hardware probe after the real-Switch direct Vulkan triangle PASS.

It deliberately tests the narrowest new boundary:

```text
Dawn / WebGPU
      ↓
Vulkan backend
      ↓
loaderless mesa-switch / NVK
      ↓
Tegra X1 GPU work
```

The probe renders a three-vertex WGSL triangle into a small offscreen WebGPU texture and waits for GPU completion. It does not create a Dawn surface or present a Dawn frame yet.

The direct Vulkan path has already proven the VI surface, swapchain presentation and the physical Switch display. This probe answers only whether pinned Dawn can discover the same NVK Vulkan device, create a WebGPU device/pipeline and execute GPU work on Horizon.

## Exact dependency pins

WiiCompiled:

```text
patchzyy/Wiicompiled
a135beb201042b20f390c6695ca6b26768820fb4
```

That revision's Aurora declares Dawn package:

```text
v20260603.191052
```

The corresponding encounter/dawn-build release records its exact google/dawn source as:

```text
13abc3bc8ea2d3c2050f9e77a12d012108ceee24
```

Mesa/NVK:

```text
danfromtico/mesa-switch
b297e230ef88c6c88df2561becf864f979f494a6
```

## Horizon patch boundary

Stock Dawn dynamically opens libvulkan.so(.1) and resolves vkGetInstanceProcAddr from that shared library. The proven Switch Mesa path is loaderless and statically linked.

The local-only dependency patch therefore:

- adds an explicit Dawn SWITCH platform classification;
- bypasses the Vulkan shared-library open on Horizon;
- resolves Dawn's GetInstanceProcAddr from mesa-switch's statically linked vk_icdGetInstanceProcAddr;
- makes Dawn's generic dynamic-library helper inert on Horizon;
- avoids Linux DMA-BUF / opaque-FD Vulkan helpers;
- supplies minimal executable/module-path behavior for Horizon.

The upstream Dawn checkout under .deps remains uncommitted. Only the auditable patcher is public.

## Why this probe does not use TimedWaitAny yet

Pinned Aurora requests wgpu::InstanceFeatureName::TimedWaitAny. Dawn's POSIX wait path uses pipe() / poll(), which is a separate Horizon portability boundary.

This first GPU proof uses:

```text
CallbackMode::AllowProcessEvents
Instance::ProcessEvents()
```

for adapter, device and queue completion. That prevents a Horizon wait primitive from being misdiagnosed as a Dawn/Vulkan/NVK failure.

After this probe passes, the exact Aurora TimedWaitAny behavior can be ported or replaced deliberately against pinned semantics.

## Build

```sh
git checkout main
git pull --ff-only
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-m3-dawn-nvk-probe.sh
```

Output:

```text
m3-dawn-probe/WiiCompiled-Switch-m3-dawn-nvk-probe.nro
```

Copy to:

```text
/switch/WiiCompiled-Switch-m3-dawn-nvk-probe/
  WiiCompiled-Switch-m3-dawn-nvk-probe.nro
```

Diagnostic report:

```text
/switch/WiiCompiled-Switch/m3-dawn-nvk-probe.txt
```

## Hardware PASS criterion

The report must show, in order:

- WebGPU instance creation;
- Vulkan adapter selection;
- adapter/device creation;
- WGSL shader module creation;
- WebGPU render-pipeline creation;
- command submission;
- successful queue completion;
- final marker:

```text
PASS DAWN_WEBGPU_VULKAN_NVK_OFFSCREEN_TRIANGLE
RESULT=PASS
```

The NRO waits for + before exiting.

## What a PASS would prove

A PASS means:

```text
Dawn/WebGPU → Vulkan → loaderless NVK → Tegra X1 GPU execution
```

works on real Switch hardware.

It would not yet mean Dawn presentation through NWindow, Aurora GX rendering, WiiCompiled HleFifoWrite rendering, or an RMCP01 / Mario Kart frame.

The next controlled step after PASS is the Dawn/Aurora presentation boundary, reusing the already hardware-proven native VI/NVK substrate.
