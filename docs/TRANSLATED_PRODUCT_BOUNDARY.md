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
- the latest accepted Discovery path crosses texture-matrix setup and all
  eight disabled coordinate triples and six TEV setters on stages 0..15, then
  stops at KColor ID 0 / guest pointer `0x80398FCC`;
  older texture/KD/audio branches remain
  recorded in dated reports.

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
exact IA8 loads on maps 0..7                ✅ hardware crossed
  ↓
ten type-0 texture matrices                ✅ hardware crossed
  ↓
Gen2/disabled Scale/Bias triples, c0..7     ✅ hardware crossed
  ↓
six TEV scalar setters, stages 0..15       ✅ default tuples returned
  ↓
GXSetTevKColor 0x80171ED4, ID 0             🟡 arrived; not returned
  ↓
recognizable Mario Kart Wii image          ❌ not proven
```

The earlier coordinate
[Discovery result](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md)
accepts `Gen2(c,1,4,60,0,125)`, `Scale(c,0,0,0)` and `Bias(c,0,0)` for c=0..7.
Restored caller `0x80241380` at dispatch 605620 retains final coordinate 7
and the Bias stage; verified control flow and the later DIRECT frontier
establish the loop returns. GXSetTevDirect stage 0 is observed at dispatch
605633, LR `0x80240F98`, 100.205 seconds from the first translated dispatch,
but has not returned. The other TEV neighbors remain static forecasts.
Enabled Scale/Bias branches and arbitrary Scale sizes remain host-tested only.

Coordinate code candidate `91a4a01`, NRO `64ba8377...`, transferred with nxlink
exit 0 at 2026-10-02 23:09:38 UTC. Its 28 reports, 526,932 bytes, include
12 changed files retrieved on October 3; the user confirmed a black screen.
The snapshot at dispatch 605367 records 1556 FIFO writes and 99 successful
presents / 0 failures before the loop. It does not measure those later native
emissions or establish recognizable pixels. See the
[bounded contract and validation record](GX_TEX_COORD_BATCH_2026-10-03.md).
Older exact LOD/wrap, KD and audio crossings remain in dated hardware reports.
They do not accept broader coordinate arguments or unreturned TEV calls.
The subsequent [audit hardware run](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md),
`b3484117` / `7ecbc8a9...`, preserves that normal path, reaching the same
Direct stage-0 frontier at dispatch 608381. The user again saw black. That
run establishes normal-path non-regression, with negative failure branches
remaining host/static evidence. The [six-setter TEV batch](GX_TEV_SCALAR_BATCH_2026-10-03.md)
passed local gates, all five GitHub workflows on integrated code `e76e8f38`,
its private build and [fresh hardware progression](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md).
The coherent later KColor boundary establishes return of all sixteen
default iterations: 96 new setter calls plus 16 existing Order calls.
KColor ID 0, pointer `0x80398FCC`, blocks at dispatch 605056 / 98.265 seconds.
The user reported a black screen and an error at exit. Alternate TEV inputs
retain host contracts only; KColor bytes and native decoding/pixels remain
unproven. Snapshot 604804 precedes all three loops and retains 1556 FIFO
writes / 99 successful presents / 0 failures, without measuring later emissions.

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

For a sustained black screen with no new blocker, `fast-track-heartbeat-history.txt` is the primary liveness diagnostic. For an exit, inspect the attributable dispatch-blocker or native-exception report first. `ACTIVE` means the translated heartbeat changed between watchdog samples; consecutive `STALE` samples mean the watchdog thread remains alive while translated execution stopped advancing.

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

The translated-product link seam, bounded disabled coordinate loop and
default six-setter TEV loop are crossed. KColor ID 0 / pointer `0x80398FCC`
is the latest accepted arrival boundary.
The separate [TEV color/table candidate](GX_TEV_COLOR_BATCH_2026-10-03.md)
now passes all five GitHub workflows and its exact private build (code
`1333b0e2`, NRO `a56be881...`). KColor and adjacent Color/SwapModeTable are
implemented with bounded memory/enum guards; fresh console return remains pending.
The next console test must establish later coherent progression beyond the
twelve color/table calls before accepting their return. Actual RGBA bytes
and recognizable game pixels remain unproven.

1. bind the copied reports to the exact candidate/NRO and retain their hashes;
2. inspect a new blocker or exception before diagnosing a timing/scheduler issue;
3. if no blocker appears, classify ACTIVE versus STALE watchdog history;
4. preserve the accepted coordinate/TEV default-loop scope and require later
   progression before accepting KColor return; arrival alone is insufficient;
5. retain the headless control target and the hardware-driven FST/DVD/input/audio
   scopes; do not extend unrelated behavior from a static forecast.


## Latest console result — TEV colors crossed (2026-10-03)

The [fresh color/table hardware result](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md)
supersedes the earlier pending color/table status. All twelve executed calls
returned through the coherent later caller; AlphaCompare `0x80172088`,
(7,0,0,7,0), is the new DIRECT hard stop. Black output and a crash persist.
Actual RGBA bytes and recognizable game pixels remain unproven. The elapsed
time includes an unexplained watchdog sampling gap, so it is not a performance
measurement. Prior dated results above retain their original scope.
