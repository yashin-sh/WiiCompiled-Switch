# M2 — Translated-product boundary

Status: **hardware-validated and crossed on real Nintendo Switch**. The project
links and executes a locally generated WiiCompiled Mario Kart Wii product,
reaches PAL `main()`, loads real user-owned resources, produces real RMCP01
FIFO work and successfully presents frames through the rendered Switch path.
The translated-product seam itself is no longer an active blocker.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Why this boundary exists

WiiCompiled is a static recompiler. A translated Mario Kart Wii product is not a DOL dynamically loaded by the Horizon runtime from the SD card.

At the pinned upstream revision the translator flow is:

1. `translate-recursive` emits translated C++ from locally supplied game inputs;
2. `generate-data-init` emits embedded data-section initialization and `RuntimeConfig.h`;
3. `emit-build-shards` emits the generated native build graph;
4. the generated output is compiled and linked together with the WiiCompiled runtime into one native executable.

For the Switch port, the user-owned translated product is therefore linked into the same AArch64 NRO as the Horizon runtime.

## Three separate data domains

### 1. Build-time user-owned inputs

Local inputs used by WiiCompiled's translator, including the DOL/REL required by the Mario Kart Wii manifest.

### 2. Build-time translated product

Generated C++ plus generated runtime configuration/data initialization compiled into the NRO.

### 3. Runtime SD data

Host/runtime state under `sdmc:/switch/WiiCompiled-Switch`, including logs, cache/configuration state, NAND backing and fast-track diagnostics.

The SD runtime directory is **not** a loader for translated executable code.

## Public weak seam

`include/translated_product.hpp` defines the translated-product query ABI. The public Nintendo-data-free build supplies a weak definition that reports no linked game product.

This preserves a public CI path that can validate the Horizon runtime without shipping or ingesting Nintendo game-derived content.

## Local strong product path

The private local build replaces the public weak product seam with the generated WiiCompiled product and associated handoff/data initialization.

That path is now hardware-validated far beyond metadata inspection:

- generated data initialization executes;
- translated execution handoff executes;
- PAL `__start` (`0x800060A4`) executes on Switch;
- PAL `main()` (`0x8000B6B0`) is reached after 605 translated dispatches;
- post-main execution progresses through `System::RKSystem::main` and `System::RKSystem::initialize`;
- HostContext-backed guest `OSThread` continuations resume interior translated continuations correctly;
- the observed VI/GX, WPAD/PAD, time, power, SC and scheduler boundaries have been hardware-crossed through `OSWakeupThread`;
- sustained translated execution has progressed far beyond the first
  37,148-dispatch run;
- user-owned FST/DVD/SZS/StaticR/Home Button resource loading is
  hardware-proven;
- real RMCP01 FIFO work reaches Aurora/Dawn/NVK;
- repeated `GXCopyDisp` / successful presents are hardware-proven;
- current blocker tracking has moved into exact Home Button texture-object and
  KD/NWC24 sequences.

The translated-product seam itself is therefore no longer an active blocker. Current work is post-main runtime/game initialization and first-frame preparation.

## Current execution milestone

```text
local user-owned game inputs
  ↓
WiiCompiled generation + AArch64 NRO link
  ↓
PAL __start / main                         ✅ hardware validated
  ↓
post-main scheduler/resource execution     ✅ hardware validated
  ↓
FST / English.szs / StaticR / Home Button ✅ hardware validated
  ↓
real RMCP01 FIFO work                      ✅ hardware validated
  ↓
GXCopyDisp / GPU present                   ✅ hardware validated
  ↓
Home Button texture-object initialization  ✅ four LOD / three wrap tuples crossed
  ↓
RVL OS alarm initialization                 ✅ OSSetPeriodicAlarm crossed
  ↓
NW4R sound-player initialization             ✅ SoundPlayer::SetVolume crossed
  ↓
GX texture-object initialization              🟡 eighth exact LOD tuple
```

As of 2026-09-28, the KD path is hardware-crossed through fd 2003 close;
PAL `AIInit (0x801240B0)`, `__AXOutInitDSP (0x801269BC)`,
`AIRegisterDMACallback (0x80123F88)`, `AIInitDMA (0x80123FCC)` and
`AIStartDMA (0x80124048)` are hardware-crossed. Seven exact `GXInitTexObjLOD` descriptors and four exact
`GXInitTexObjWrapMode` tuples are hardware-crossed. `OSSetPeriodicAlarm
(0x801A08E0)` and `nw4r::snd::SoundPlayer::SetVolume (0x800A35E0)` are also
hardware-crossed. The fifth, sixth and seventh `GXInitTexObjLOD` descriptors on
`obj=0x908FA4E0` / `obj=0x908FA5C0` / `obj=0x907938A0` and the fourth
`GXInitTexObjWrapMode` tuple on `obj=0x908FA4E0` are hardware-crossed.
The current exact gate is the eighth `GXInitTexObjLOD` descriptor on
`obj=0x908FA820`, while real `revo_kart.brsar` and Home Button resources
remain healthy. The visually correct Mario Kart Wii image is still unproven.

## Important boundary lessons from hardware

### Native HLE can call translated guest code

Pinned WiiCompiled HLE is not always a leaf. `IPCCltInit` calls translated `IPCInit` before publishing its guest-visible state. The Switch port therefore preserves native-to-translated handoff instead of treating every native override as isolated.

### Native HLE can require guest-visible bookkeeping even when host hardware I/O is skipped

Examples already crossed include `__OSInitSTM`, NAND state, VI state, power callback state and scheduler queues. Hardware-facing host work may be collapsed or replaced while guest-visible state still has to match the pinned runtime contract.

### Guest thread resume is a host-context problem, not just an address-dispatch problem

Hardware exposed saved SRR0 `0x80238A78`, an interior continuation inside translated code. The Switch runtime now preserves guest `OSThread` continuations through host `HostContext` fibers so the original translated host stack resumes correctly.

### Black output does not prove a translated stall or renderer absence

The headless control target still uses a FIFO sink, but the separate rendered
target is now hardware-proven to decode real RMCP01 FIFO work and present
frames successfully. The independent Horizon watchdog remains useful for
separating guest liveness/scheduler stalls from graphics-path behavior.

## Diagnostics at this boundary

The local translated path uses durable SD records rather than relying on visible output:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-progress.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat-history.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-main-reached.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-exception.txt
```

These files distinguish:

- translated execution is still progressing;
- the independent watchdog is alive but translated dispatch has become stale;
- a specific unsupported direct/indirect boundary stopped execution;
- a host exception occurred with attributable AArch64/guest context.

`fast-track-main-reached.txt` is the durable proof marker for PAL `main` at `0x8000B6B0`.

For the current sustained-black-screen frontier, `fast-track-heartbeat-history.txt` is the primary diagnostic. `ACTIVE` means the translated heartbeat changed between watchdog samples; consecutive `STALE` samples mean the watchdog thread remains alive while translated execution stopped advancing.

## Hardware history summary

### 2026-09-10

The public Nintendo-data-free boundary was validated on hardware: Horizon services, guest memory, HostContext, filesystem roots and the weak translated-product seam behaved as expected.

### 2026-09-12 / 2026-09-13

The local generated-product path crossed the early Wii SDK/OS/NAND/DVD startup sequence and reached PAL `main()` on real hardware.

### 2026-09-14 through 2026-09-16

The post-main path advanced through MEM2 allocation, mutex/thread setup, VI/GX initialization, guest scheduler/context switching, HostContext-backed guest continuation, WPAD/PAD initialization, `OSGetTime`, `OSSetPowerCallback` and `SCGetProductArea`.

### 2026-09-17

Hardware crossed `OSWakeupThread` and then no longer hit an immediate unsupported-dispatch abort. The run reached 37,148 translated dispatches total / 36,543 post-main and was manually terminated while black. The last durable target mapped to the RMCP01 EGG AsyncDisplay range.

See `HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md` and `HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`.

## Local-only content policy

Do not commit or upload:

- DOL/REL game inputs;
- disc images;
- Nintendo keys or firmware;
- extracted copyrighted assets;
- generated translated game C++/data/object output;
- game-containing NRO/ELF artifacts.

Only Nintendo-data-free runtime/platform code, documentation and synthetic probes belong in public CI.

## Validation result

- pinned WiiCompiled submodule: PASS;
- Nintendo-data-free public NRO path: PASS;
- real Switch runtime/GuestFlat/HostContext foundation: PASS;
- local generated data initialization: PASS;
- local translated `__start` execution: PASS;
- PAL `main()` reached: **PASS on hardware**;
- HostContext guest continuation: **PASS on hardware**;
- observed post-main scheduler/input/time/SC sequence through `OSWakeupThread`: **PASS on hardware**;
- sustained post-main translated execution: **PASS on hardware**;
- EGG AsyncDisplay range reached: **PASS on hardware**;
- active-loop vs durable-stall classification: **PASS on hardware**;
- real GX → Switch rendered path: **PASS on hardware for FIFO work and GPU present**;
- visually correct Mario Kart Wii image: **NOT YET PROVEN**;
- first rendered frame: **NOT YET PROVEN**.

## Next boundary

The meaningful boundary is now post-main liveness rather than translated-product linkage:

1. run the current local fast-track on real hardware;
2. if the display remains black, leave it running long enough to collect the independent watchdog history;
3. inspect `fast-track-heartbeat-history.txt` to classify ACTIVE vs STALE behavior;
4. if a new unsupported dispatch or exception appears, attribute that exact boundary against the pinned WiiCompiled revision;
5. use the already-published user-owned FST and installed #185 DVD bridge without extending them unless hardware reaches a new resource boundary;
6. use `fast-track-thread-events.txt` to identify later OSThread `0x90112660` before changing scheduler or renderer behavior.
