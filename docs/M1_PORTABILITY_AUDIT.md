# M1 — WiiCompiled portability audit for Nintendo Switch

Audited upstream: `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`

## Executive result

A native Horizon/libnx port is technically plausible, but it is not a simple rebuild of the Linux AArch64 target. The static-recompiled game code already supports AArch64 as an architecture; the main work is replacing host-OS facilities currently provided by Windows/Linux/macOS and adapting Aurora's SDL3/WebGPU graphics stack to Switch homebrew APIs.

The three highest-risk areas are:

1. **Guest virtual memory / page-fault model**
2. **Cooperative guest contexts / fibers**
3. **Graphics backend and presentation path**

The current `.nro` bootstrap proves our devkitA64/libnx toolchain is healthy, but does not yet link WiiCompiled.

## Build/platform gate

`runtime/CMakeLists.txt` currently recognizes Windows, macOS and Linux. Linux includes both x86_64 and AArch64, which is encouraging, but Horizon is rejected by the platform gate.

### Switch work

- Add an explicit `MKW_PLATFORM_SWITCH`/Horizon target.
- Cross-compile the runtime with devkitA64.
- Split reusable AArch64 code from Linux/Darwin OS code.
- Avoid pretending Horizon is Linux: it does not provide the same VM/signal/windowing environment.

## 1. Guest memory — critical

WiiCompiled's fast path reserves a **flat 4 GiB Wii guest address space** at a fixed host virtual address, with a second host alias. Page protections are also used as part of the MMIO/deferred-read/executable-write interception mechanism.

On generic AArch64 the current fixed guest base is `0x0000001000000000` (64 GiB). This was specifically chosen to fit 39-bit Linux AArch64 hosts.

### Why this is hard on Horizon

The generic implementation assumes OS VM primitives and a recoverable access-fault mechanism. Horizon/libnx has different memory-management APIs and cannot be assumed to reproduce POSIX `mmap`/`mprotect` + signal semantics directly.

### First experiment

Before integrating the full runtime, write a standalone Switch VM probe that answers:

- Can a 4 GiB contiguous virtual range be reserved at a deterministic address?
- Can MEM1/MEM2-style backing be mapped into two aliases?
- What page sizes/alignment constraints apply?
- Can guest pages be toggled between readable/writable/inaccessible cheaply?
- What fault information, if any, can an NRO recover in-process?

If the dual-view fault-driven model cannot be reproduced, we need a Switch-specific checked-memory path or a redesigned fast path.

**Risk: HIGH.** This can materially affect performance because every translated load/store is hot-path code.

## 2. HostContext / fibers — high, but bounded

The guest scheduler uses a deliberately small `HostContext` abstraction.

Existing implementations:

- Windows: native fibers
- Linux: `libco`
- Apple Silicon: custom AArch64 context-switch assembly plus `mmap` stacks/guard pages

The Apple Silicon backend proves WiiCompiled already has a hand-written AArch64 cooperative context ABI. That is valuable, but the file is Darwin-specific: stack allocation uses `mmap`/`mprotect`/`munmap`, and the assembly has Darwin ABI details (including treatment of `x18` and Mach-O symbol naming).

### Switch approach

Create a Horizon backend with:

- libnx-compatible stack allocation
- a Switch/AAPCS64 context-switch assembly companion derived conceptually from the existing AArch64 implementation
- preserved callee-saved GPRs/SIMD registers
- 16-byte stack alignment
- a standalone context ABI test before wiring `FiberManager`

**Risk: MEDIUM-HIGH.** The abstraction is small and therefore tractable.

## 3. Graphics / Aurora — critical

Aurora currently provides:

- an SDL3 application/window/input layer
- GX compatibility layer
- D3D12, Vulkan and Metal graphics support
- WebGPU/Dawn integration in the current runtime path

There is no upstream Horizon backend.

### Switch options

#### A. Deko3D backend

Deko3D is the established low-level GPU API in the devkitPro/libnx ecosystem. A dedicated Aurora/Deko3D backend is likely the most controlled long-term solution, but it is substantial work because the existing GX implementation is built around Aurora's current GPU abstraction.

#### B. Vulkan/NXVK experiment

As of mid-2026, a Horizon-native Vulkan driver (`NXVK`) is emerging in the Switch homebrew ecosystem. This is strategically interesting because WiiCompiled/Aurora already has a Vulkan path. It should be treated as **experimental**, not as the only architecture until feature coverage, Dawn compatibility and performance are verified on hardware.

#### C. SDL layer reuse

Aurora is heavily coupled to SDL3 for window/events/input/audio/file helpers. Switch homebrew has mature SDL2 packaging, while Aurora specifically targets SDL3. We need to verify whether a usable SDL3/libnx port exists; otherwise the cleanest path may be to bypass SDL for the Switch target and provide Horizon-specific input/audio/window glue.

**Risk: HIGH.** GPU strategy may determine whether the project is practical on Tegra X1.

## 4. Input — low/medium

Our bootstrap already reads standard Switch controllers through libnx HID. Aurora/WiiCompiled currently consumes SDL gamepad/input APIs.

Recommended design:

`libnx HID -> switch input adapter -> Aurora/Wii controller state`

Do not force SDL into the port just for controller support.

**Risk: LOW.** Joy-Con/Pro Controller mapping is straightforward compared with VM/GPU work.

## 5. Audio — medium

WiiCompiled's `AudioBackend` currently includes SDL3 audio directly.

Switch should get an explicit audio backend using Horizon/libnx audio services (Audren path) rather than making the core runtime depend on SDL3 availability.

**Risk: MEDIUM.** Buffering and latency will need hardware tuning, but this is not expected to block first boot.

## 6. Filesystem / user data — medium

The Switch port should use an application root such as:

`sdmc:/switch/WiiCompiled-Switch/`

with separate local-only/generated content directories. Game extraction remains a host-side/user-owned-dump step; copyrighted extracted assets must never be committed.

`std::filesystem` compatibility under devkitA64 should be tested early because Aurora uses it in multiple places.

**Risk: MEDIUM-LOW.** Mostly platform plumbing.

## 7. Threads, synchronization and time — medium

Much of the C++ standard threading layer may work under devkitA64/newlib, but assumptions around native thread APIs, TLS and timing must be tested rather than inherited from Linux.

Provide platform wrappers for:

- monotonic ticks
- sleep/yield
- thread creation/priority/affinity where required
- mutex/condition behavior used by the runtime

**Risk: MEDIUM.** The Tegra X1 makes unnecessary synchronization particularly expensive.

## 8. Networking — defer

Offline boot/racing should come first. Sockets/RetroWFC/Retro Rewind compatibility belongs after memory, scheduler, graphics, input and audio work.

## Recommended implementation order

1. Keep the minimal `.nro` bootstrap green.
2. Build **VM probe** on real Switch hardware.
3. Build **AArch64 HostContext probe** on Switch.
4. Add Horizon target to a minimal subset of WiiCompiled runtime.
5. Decide graphics route using a small Aurora/Deko3D vs Vulkan/NXVK feasibility spike.
6. Port input and audio adapters.
7. Link translated runtime without game assets in repository.
8. Reach first boot/menu.
9. Profile before adding online/mod compatibility.

## Current feasibility verdict

**Proceed.** There is enough AArch64 and platform separation upstream to justify the port. However, a successful `.nro` build should not be confused with a performant game port. The flat guest-memory model and the graphics stack are the two project-defining technical risks.
