# M2 — Horizon runtime bootstrap / SDL decoupling

Status: **core bootstrap hardware-validated on real Nintendo Switch (2026-09-10)**. The translated-product boundary added after that validation still requires its own hardware report.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Reach a stable WiiCompiled runtime boundary on Horizon without constructing a desktop SDL window or starting Aurora graphics/audio, then expose the correct build-time seam for a translated Mario Kart Wii product.

The validated core sequence is:

1. initialize native Horizon host services;
2. initialize the Wii guest memory model;
3. exercise WiiCompiled's real `HostContext` public API through the Switch AArch64 implementation;
4. expose stable SD-card runtime-data roots;
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
- explicit runtime-data roots: `Logs/`, `Cache/`, `Config/`, `NAND/`;
- audio: explicit unsupported/stub result (no silent SDL fallback);
- graphics: explicit stub state (Aurora/Deko3D not initialized here).

Runtime SD data is separate from translated code. None of these directories is used as a DOL or translated-product loader.

## Runtime smoke path

The NRO keeps the existing M2 probes, then runs `runtime_bootstrap::start()`:

- Horizon services ready;
- `Memory::Init(Memory::Config::WiiDefaults())` active;
- WiiCompiled upstream `HostContext` scheduler initialized;
- worker context created through the public API;
- first handoff verified;
- continuation verified;
- graphics/audio remain stubbed;
- translated-product ABI seam inspected without side effects.

The public Nintendo-data-free build supplies only the weak product stub, so the expected current stop point is:

`WAITING_FOR_TRANSLATED_PRODUCT`

If a future local build links an ABI-compatible strong product adapter, the current slice can report:

`TRANSLATED_PRODUCT_LINKED`

That state still does not initialize generated data sections or execute translated Mario Kart Wii code.

The worker is destroyed after the two API-level context handoffs; the scheduler and Wii memory remain live until `runtime_bootstrap::stop()` at application shutdown.

## Hardware validation — 2026-09-10

The returned real-hardware report for the previous bootstrap revision reported:

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
- historical temporary stop point: `WAITING_FOR_USER_DATA`.

The accompanying VM regression report also passed the 4 GiB guest reservation, `svcMapMemory` regression path, SharedMemory dual mapping, 100000/100000 context-switch stress test, checked GuestFlat, upstream GuestFlat API, and `Memory::Init` smoke test.

See `HARDWARE_RESULTS_2026-09-10.md` for that recorded evidence.

The new translated-product boundary intentionally changes the stop-point/report contract and therefore requires a fresh hardware run before issue #18 can close.

## Correct architecture: translated product is build-time

At the pinned upstream revision, WiiCompiled's translator performs:

1. `translate-recursive` — emits translated C++ from the user-owned DOL;
2. `generate-data-init` — emits embedded `.data`/`.rodata`/`.sdata` initialization and `RuntimeConfig.h`;
3. `emit-build-shards` — emits the generated build graph;
4. CMake/Ninja — links the generated output and `runtime/` into one native executable.

For the MKWii project, the upstream manifest reads locally supplied `Assets/main.dol` / `Assets/StaticR.rel` and writes generated output under `generated/`.

Therefore the Switch port keeps three concepts distinct:

- **build-time user-owned source inputs** used locally by the translator;
- **build-time generated translated product** linked into the NRO;
- **runtime SD data** for configuration, NAND/save-compatible state, logs/cache and later runtime/mod assets where appropriate.

Game-derived source inputs and generated translated output remain excluded from this repository and CI.

The concrete ABI seam and validation rules are documented in `TRANSLATED_PRODUCT_BOUNDARY.md`.

## Diagnostics

The runtime writes:

`sdmc:/switch/WiiCompiled-Switch/runtime-bootstrap.txt`

For the public build, the new report should include:

```text
translated product     : NOT LINKED
translated product ABI : expected=1 reported=0
translated product id  : <none>
translated build       : Nintendo-data-free stub
stop point             : WAITING_FOR_TRANSLATED_PRODUCT
hardware validation    : REQUIRED (attach runtime-bootstrap.txt)
```

CI proves only the source pin and native compile/link path. A fresh returned Switch report is required to validate this new boundary on hardware.

## Next slices

1. hardware-validate the translated-product boundary and close #18;
2. create the local-only generated-product adapter/build path;
3. initialize generated data sections and reach the first translated entry-point handoff;
4. port guest timing/thread/synchronization dependencies exposed by real execution;
5. complete filesystem/NAND runtime-data abstractions and replace remaining runtime input SDL consumers;
6. implement Audren audio;
7. begin the Deko3D/Aurora GX graphics spike only after the pre-graphics runtime path is stable.
