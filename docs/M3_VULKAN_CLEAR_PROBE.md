# M3 Vulkan clear-frame probe

Tracking: #4, #162

## Purpose

This target is the first isolated graphics proof after the 2026-09-18 hardware run established that the normal #117 translated runtime remains active through the VI/post-retrace loop.

It deliberately does **not** contain Mario Kart Wii translated code and does **not** replace the normal fast-track GX FIFO sink.

The probe tests only:

```text
libnx / Horizon NWindow
        ↓
VK_NN_vi_surface
        ↓
mesa-switch loaderless NVK
        ↓
VkSwapchainKHR
        ↓
clear acquired image
        ↓
QueuePresentKHR
        ↓
Switch display
```

If this works on hardware, the Switch Vulkan/NVK/VI presentation layer is proven independently of WiiCompiled, Aurora and Dawn.

## Pinned graphics dependency

The local build script pins:

```text
danfromtico/mesa-switch
b297e230ef88c6c88df2561becf864f979f494a6
```

That revision is Mesa 26.2.2-based and exposes a loaderless static NVK archive plus the `VK_NN_vi_surface` NWindow WSI.

The dependency is cloned under `.deps/m3/` and is never committed.

## Build

From the WiiCompiled-Switch repository root:

```sh
git checkout main
git pull
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-m3-vulkan-clear-probe.sh
```

The first invocation builds the pinned mesa-switch NVK stack in Docker. Later invocations reuse the local archive unless:

```sh
MKW_M3_FORCE_MESA_REBUILD=1 \
MKW_JOBS=4 \
bash scripts/build-m3-vulkan-clear-probe.sh
```

Output:

```text
m3-graphics-probe/WiiCompiled-Switch-m3-vulkan-clear-probe.nro
```

## Hardware run

Copy the NRO to the SD card, for example:

```text
/switch/WiiCompiled-Switch-m3-vulkan-clear-probe/
  WiiCompiled-Switch-m3-vulkan-clear-probe.nro
```

Launch it through hbmenu in application/title-override mode.

Expected PASS:

- a continuously changing full-screen color is visible;
- the app keeps presenting until `+` is pressed;
- the report contains `PASS FIRST_PRESENT` and continuing `ACTIVE frames=...` records.

Report:

```text
/switch/WiiCompiled-Switch/m3-vulkan-clear-probe.txt
```

If the screen remains black or the NRO exits, the report identifies the exact Vulkan stage/result code that failed.

## What this does not prove

A PASS does not yet prove:

- Dawn/WebGPU works on Horizon;
- Aurora GX works on Horizon;
- WiiCompiled `HleFifoWrite` can render on Switch;
- RMCP01 can render a frame.

Those are the next #162 steps in that order:

1. clear-frame NVK/VI proof — this target;
2. Vulkan triangle;
3. Dawn/WebGPU on the proven Vulkan/NVK surface;
4. fabricated Nintendo-data-free GX/FIFO traffic through pinned `HleFifoWrite`;
5. private local RMCP01 GX stream.

The normal #117 fast-track remains unchanged while these probes are isolated.
