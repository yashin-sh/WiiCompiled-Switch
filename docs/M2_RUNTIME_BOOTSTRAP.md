# M2 — Horizon runtime bootstrap / SDL decoupling

Status: implementation slice pending CI and real-hardware validation.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Reach the first stable WiiCompiled runtime boundary on Horizon without constructing a desktop SDL window or starting Aurora graphics/audio. This slice intentionally stops before translated Mario Kart Wii data is loaded.

The target sequence is:

1. initialize native Horizon host services;
2. initialize the hardware-validated Wii guest memory model;
3. exercise WiiCompiled's real `HostContext` public API through the Switch AArch64 context implementation;
4. expose a stable SD-card application root and user-data boundary;
5. stop cleanly with graphics and audio explicitly stubbed.

## Upstream integration

`third_party/WiiCompiled` is now a real Git submodule pinned to the audited WiiCompiled SHA. The Switch build consumes upstream code rather than copying or renaming the runtime contracts.

The first upstream translation unit compiled into the NRO is:

- `runtime/src/platform/host_platform.cpp`

The Switch-specific cooperative-context implementation includes the real upstream header:

- `runtime/include/host_context.h`

and implements the exact `HostContext::{InitializeScheduler,ShutdownScheduler,Create,Destroy,IsCurrent,Switch}` contract using the already validated `mkw_switch_co_init` / `mkw_switch_co_switch` assembly.

## SDL3 dependency inventory

At the pinned upstream revision, SDL is not a single dependency at the application boundary. Important direct dependencies include:

| Area | Upstream path | SDL dependency | Switch direction |
| --- | --- | --- | --- |
| process entry | `aurora-main/lib/main.cpp` | `SDL_main` | not used by Horizon entry |
| Aurora lifecycle/input | `aurora-main/lib/input.cpp`, `aurora-main/include/aurora/event.h` | events/gamepads/haptics | libnx lifecycle + HID |
| runtime audio | `runtime/include/audio_backend.h`, `runtime/src/audio_backend.cpp` | SDL audio/init | Audren adapter later |
| HLE controller | `runtime/src/hle/input/pad.cpp` | SDL gamepad | libnx HID adapter later |
| input bindings | `runtime/src/input_bindings.cpp` | SDL gamepad | platform input provider later |
| card/filesystem helpers | `aurora-main/lib/card/FileIO.*` | SDL IO/filesystem | stdio/libnx/SD filesystem |
| UI | `aurora-main/lib/imgui*.cpp` | SDL events/render/IO | deferred with graphics spike |
| graphics/window | Aurora Dawn/backend files | SDL video/window + Dawn/WebGPU | deferred; Deko3D spike is M3 |

The critical M2 Switch bootstrap deliberately links none of those SDL-dependent units.

## Horizon host-services boundary

`horizon_runtime_services` currently provides:

- lifecycle: `appletMainLoop()` and clean exit;
- input: libnx `PadState`, buttons and both analog sticks;
- timing: `armGetSystemTick()`, `armGetSystemTickFreq()`, `svcSleepThread()`;
- filesystem: stable `sdmc:/switch/WiiCompiled-Switch` application root through upstream `RuntimePlatform`;
- audio: explicit unsupported/stub result (no silent SDL fallback);
- graphics: explicit stub state (Aurora/Deko3D not initialized here).

The user-data boundary is reported as:

`sdmc:/switch/WiiCompiled-Switch/game-data`

Its presence is diagnostic only in this slice. No game loader is claimed or executed yet.

## Runtime smoke path

The NRO keeps the existing M2 probes, then runs `runtime_bootstrap::start()`:

- Horizon services ready;
- `Memory::Init(Memory::Config::WiiDefaults())` active;
- WiiCompiled upstream `HostContext` scheduler initialized;
- worker context created through the public API;
- first handoff verified;
- continuation verified;
- graphics/audio remain stubbed;
- stop point becomes `WAITING_FOR_USER_DATA` only if the core checks complete.

The worker is destroyed after the two API-level context handoffs; the scheduler and Wii memory remain live until `runtime_bootstrap::stop()` at application shutdown.

## Diagnostics

The runtime writes:

`sdmc:/switch/WiiCompiled-Switch/runtime-bootstrap.txt`

The report includes the upstream SHA, service states, Memory/HostContext states, the local user-data root, and an explicit `hardware validation: REQUIRED` line.

CI success means only that the submodule pin is correct and the NRO compiles/links. It is **not** hardware validation.

## Hardware validation procedure

Launch through hbmenu using title override / application mode with full memory, not Album applet mode. Run the NRO, exit with `+`, then return `runtime-bootstrap.txt` together with `vm-probe.txt` if any earlier M2 regression appears.

Expected target for this slice, pending hardware evidence:

`stop point : WAITING_FOR_USER_DATA`

Do not mark this hardware-validated from CI output alone.

## Next slices

After this bootstrap is hardware-confirmed:

1. widen the actual runtime source subset needed before Aurora;
2. port filesystem/game-data discovery contracts;
3. port guest timing/thread/synchronization dependencies;
4. replace runtime input SDL consumers with the libnx provider;
5. implement Audren audio;
6. only then begin the Deko3D/Aurora GX graphics spike.
