# Hardware result: M3 NVK / VI clear-frame PASS

Date: 2026-09-18  
Tracking: #4, #162  
Implementation: #165

## Hardware observation

The isolated `WiiCompiled-Switch-m3-vulkan-clear-probe.nro` was built locally and launched on a real Nintendo Switch through hbmenu title override / application mode.

Observed result:

- full-screen colors were visibly presented and alternated continuously;
- the probe remained active rather than returning immediately to hbmenu;
- pressing `+` exited the probe cleanly.

This is a hardware PASS for the isolated presentation path:

```text
libnx / Horizon NWindow
        ↓
VK_NN_vi_surface
        ↓
mesa-switch loaderless NVK
        ↓
VkSwapchainKHR
        ↓
GPU clear
        ↓
QueuePresentKHR
        ↓
Switch display
```

The visual result itself is sufficient to prove that the clear/present path reached the physical display. The durable SD report was not required to classify this PASS because the expected changing-color output and clean `+` exit were directly observed.

## What is now proven

- Horizon/libnx `NWindow` presentation works for this probe;
- `VK_NN_vi_surface` surface creation is viable on the target hardware;
- loaderless mesa-switch/NVK can submit and present on the Tegra X1;
- the Vulkan swapchain path can continuously acquire, clear and present frames;
- the previous #117 black screen is not caused by an inability of the Switch to present GPU output.

## What is not yet proven

This does **not** yet prove:

- a Vulkan graphics pipeline / triangle;
- Dawn/WebGPU on Horizon;
- Aurora GX on Horizon;
- WiiCompiled `HleFifoWrite` driving visible output;
- an RMCP01/Mario Kart Wii frame.

The next #162 hardware milestone is therefore a simple Vulkan triangle using the already-proven VI/NVK swapchain.

## Linux build findings

The first successful Linux build also exposed four reproducibility issues in the initial #165 helper:

1. SELinux Enforcing blocked the Docker bind-mount/build path.
2. `MESA_SWITCH_RUST_TARGET` was not propagated, allowing Rust objects with the host architecture to enter `libvulkan.a`.
3. the native/cross Rust split must keep native proc-macro/host tools on the real host `rustc` while the cross Meson compiler uses the wrapper;
4. the probe Makefile incorrectly required Switch `libelf`, and the final NRO link needed the AArch64 Rust standard-library closure used by Mesa's Rust objects.

The follow-up build fix keeps the exact mesa-switch pin and applies a narrow local integration patch:

- dedicated Docker image with `aarch64-unknown-linux-gnu` installed;
- explicit `MESA_SWITCH_RUST_TARGET=aarch64-unknown-linux-gnu`;
- SELinux isolation handled per-container with Docker `--security-opt label=disable`, avoiding a host-wide `setenforce 0`;
- native host tools remain on native `rustc`; only cross setup/build sees the wrapper;
- no `-lelf` in the probe link;
- the 19-library Rust AArch64 std closure is discovered from the container sysroot at build time;
- `m3-graphics-probe/build/` is ignored.

No generated Nintendo game data is involved in this probe.
