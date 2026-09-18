# M3 Dawn/WebGPU clear probe

Tracking: #4, #162

## Purpose

This probe is the first isolated proof above the hardware-validated direct Vulkan/NVK path.

Already proven on real Switch hardware:

```text
libnx / NWindow
      ↓
VK_NN_vi_surface
      ↓
loaderless mesa-switch / NVK
      ↓
VkSwapchainKHR
      ↓
direct Vulkan clear + triangle
```

This probe inserts Dawn/WebGPU while keeping the lower half fixed:

```text
WebGPU API
      ↓
Dawn Vulkan backend
      ↓
loaderless mesa-switch / NVK
      ↓
VK_NN_vi_surface
      ↓
NWindow / physical Switch display
```

No Mario Kart Wii code, assets, translated product or Nintendo-derived fixture is involved.

## Pinned dependencies

Dawn Switch fork:

```text
danfromtico/dawn-switch
77029ea85250c9bdddfc2f88034afb6b5356a031
```

Mesa/NVK:

```text
danfromtico/mesa-switch
b297e230ef88c6c88df2561becf864f979f494a6
```

The Dawn fork already provides the Switch platform pieces needed by this spike:

- `DAWN_PLATFORM_IS(SWITCH)`;
- `SurfaceSourceSwitchNWindow`;
- `VK_USE_PLATFORM_VI_NN`;
- `vkCreateViSurfaceNN` surface creation;
- loaderless Vulkan bootstrap through `vk_icdGetInstanceProcAddr`;
- Switch-specific runtime/TLS adaptations.

The repository does not vendor Dawn. The local builder checks out the exact pin under ignored `.deps/m3/`.

## Local integration delta

The public Dawn fork's sample CMake linkage assumes a different NVK package shape. WiiCompiled-Switch's proven Mesa build requires the AArch64 Rust standard-library closure used by NAK and does not use the sample's hard-coded `drm_nouveau` link.

`patches/dawn-switch/m3-static-nvk-link.patch` therefore changes only the isolated sample link target:

- wraps Dawn + NVK + the Rust closure in a linker group;
- accepts `DAWN_SWITCH_EXTRA_LIBRARIES`;
- removes the hard-coded `drm_nouveau` dependency;
- keeps the ICD entry points alive for static linking.

The normal #117 fast-track is untouched.

## Build

```sh
git checkout main
git pull --ff-only
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-m3-dawn-probe.sh
```

Output:

```text
m3-dawn-probe/WiiCompiled-Switch-m3-dawn-clear-probe.nro
```

The first build is intentionally heavier than the direct Vulkan probes because Dawn fetches and builds its pinned source dependencies. Subsequent builds reuse `.deps/m3/dawn-switch-build`.

## Hardware PASS criterion

Copy to:

```text
/switch/WiiCompiled-Switch-m3-dawn-clear-probe/
  WiiCompiled-Switch-m3-dawn-clear-probe.nro
```

Launch through hbmenu title override / application mode.

Expected PASS:

- Dawn creates a WebGPU instance;
- the Vulkan adapter is selected;
- a WebGPU device is created;
- `SurfaceSourceSwitchNWindow` configures a 1280x720 presentation surface;
- the full-screen clear color changes continuously;
- `+` exits cleanly;
- the persistent report ends in `RESULT=PASS`.

Report:

```text
/switch/WiiCompiled-Switch/m3-dawn-clear-probe.txt
```

Important milestones in the report:

```text
STAGE CREATE_INSTANCE PASS
STAGE CREATE_SURFACE PASS
STAGE REQUEST_ADAPTER PASS
STAGE REQUEST_DEVICE PASS
STAGE CONFIGURE_SURFACE PASS
PASS FIRST_DAWN_PRESENT
...
RESULT=PASS
```

## What a PASS proves

A hardware PASS proves Dawn/WebGPU can drive the already-proven Switch Vulkan/NVK/VI path.

It still does **not** prove:

- Aurora GX;
- pinned WiiCompiled `HleFifoWrite`;
- fabricated GX FIFO rendering;
- RMCP01/Mario Kart rendering.

After this clear probe passes, the next isolated step is a Dawn/WGSL triangle. Only after that do we insert Aurora GX and Nintendo-data-free synthetic FIFO traffic.
