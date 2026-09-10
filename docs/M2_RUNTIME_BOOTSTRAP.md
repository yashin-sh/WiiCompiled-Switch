# M2 — Horizon runtime bootstrap / SDL decoupling

Status: **hardware-validated on real Nintendo Switch (2026-09-10)**.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Reach the first stable WiiCompiled runtime boundary on Horizon without constructing a desktop SDL window or starting Aurora graphics/audio. This slice intentionally stops before a translated Mario Kart Wii product is linked/executed.

The validated sequence is:

1. initialize native Horizon host services;
2. initialize the Wii guest memory model;
3. exercise WiiCompiled's real `HostContext` public API through the Switch AArch64 context implementation;
4. expose a stable SD-card application-data root;
5. stop cleanly with graphics and audio explicitly stubbed.

## Upstream integration

`third_party/WiiCompiled` is a Git submodule pinned to the audited WiiCompiled SHA. The Switch build consumes upstream code rather than copying or renaming runtime contracts.

The first upstream translation unit compiled into the NRO is:

- `runtime/src/platform/host_platform.cpp`

The Switch-specific cooperative-context implementation includes the real upstream header:

- `runtime/include/host_context.h`

and implements the exact `HostContext::{InitializeScheduler,ShutdownScheduler,Create,Destroy,IsCurrent,Switch}` contract using the hardware-validated `mkw_switch_co_init` / `mkw_switch_co_switch` assembly.

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

## Runtime smoke path

The NRO keeps the existing M2 probes, then runs `runtime_bootstrap::start()`:

- Horizon services ready;
- `Memory::Init(Memory::Config::WiiDefaults())` active;
- WiiCompiled upstream `HostContext` scheduler initialized;
- worker context created through the public API;
- first handoff verified;
- continuation verified;
- graphics/audio remain stubbed;
- temporary stop point reaches `WAITING_FOR_USER_DATA` only if the core checks complete.

The worker is destroyed after the two API-level context handoffs; the scheduler and Wii memory remain live until `runtime_bootstrap::stop()` at application shutdown.

## Hardware validation — 2026-09-10

The returned real-hardware `runtime-bootstrap.txt` reported:

- critical SDL path: `NONE`;
- lifecycle: `READY`;
- filesystem: `READY`;
- timing: `READY`;
- libnx HID: `READY`;
- `Memory::Init`: `READY`;
- HostContext scheduler: `READY`;
- first handoff: `READY`;
- continuation: `READY`;
- audio: `STUBBED`;
- graphics: `STUBBED`;
- stop point: `WAITING_FOR_USER_DATA`.

The accompanying VM regression report also passed the 4 GiB guest reservation, `svcMapMemory` regression path, SharedMemory dual mapping, 100000/100000 context-switch stress test, checked GuestFlat, upstream GuestFlat API, and `Memory::Init` smoke test.

See `HARDWARE_RESULTS_2026-09-10.md` for the recorded evidence.

## Architecture correction: translated product is build-time

The `WAITING_FOR_USER_DATA` name is only a temporary bootstrap marker and must **not** become a runtime DOL loader design.

At the pinned upstream revision, WiiCompiled's translator performs:

1. `translate-recursive` — emits translated C++ from the user-owned DOL;
2. `generate-data-init` — emits embedded `.data`/`.rodata`/`.sdata` initialization and `RuntimeConfig.h`;
3. `emit-build-shards` — emits the generated build graph;
4. CMake/Ninja — links the generated output and `runtime/` into one native executable.

For the MKWii project, the upstream manifest reads locally supplied `Assets/main.dol` / `Assets/StaticR.rel` and writes generated output under `generated/`.

Therefore the Switch port must keep three concepts distinct:

- **build-time user-owned source inputs** used locally by the translator;
- **build-time generated translated product** linked into the NRO;
- **runtime SD data** for configuration, NAND/save-compatible state, logs/cache and later runtime/mod assets where appropriate.

Game-derived source inputs and generated translated output remain excluded from this repository and CI.

## Diagnostics

The runtime writes:

`sdmc:/switch/WiiCompiled-Switch/runtime-bootstrap.txt`

CI success proves the pin and native build/link path. The 2026-09-10 returned report additionally proves this bootstrap path on real hardware.

## Next slices

1. define the translated-product boundary and replace the misleading `WAITING_FOR_USER_DATA` runtime assumption (#18);
2. port guest timing/thread/synchronization dependencies;
3. complete filesystem/NAND runtime-data abstractions;
4. replace remaining runtime input SDL consumers with the libnx provider;
5. implement Audren audio;
6. only then begin the Deko3D/Aurora GX graphics spike.
