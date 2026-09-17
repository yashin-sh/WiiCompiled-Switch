# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Project progress

**Estimated progress toward first rendered frame: ~50%**

```text
██████████░░░░░░░░░░ 50%
```

> This percentage is an engineering estimate, not a function-count metric. The runtime has now crossed PAL `main()`, the observed post-main OS/VI/WPAD/PAD/time/thread boundaries, and reached sustained translated execution in the EGG display subsystem. A real GX → Switch renderer still does not exist, so the first-frame estimate remains deliberately conservative.

## Current status

The project executes real WiiCompiled-translated Mario Kart Wii code on real Switch hardware and has **reached PAL `main()` (`0x8000B6B0`) after 605 translated dispatches**.

The post-`main` fast-track tracked in issue #117 has now hardware-crossed the observed path through guest thread/context switching, VI/GX bootstrap, WPAD/PAD initialization, timing, power-callback state, console-area lookup, and `OSWakeupThread`.

The latest hardware run is qualitatively different from the earlier blocker-driven runs: it no longer stopped on a new unsupported dispatch or returned automatically to hbmenu. It reached **37,148 translated dispatches total, including 36,543 after `main()`**, and remained on a black screen until manually terminated. The last durable target was `0x8020FCD4`, which RMCP01 mapping places at the start of the `egg/core/eggAsyncDisplay.cpp` text range.

That is evidence of sustained translated/display-subsystem execution, **not proof of a rendered frame**. The GX FIFO bridge remains a sink, so black output is still expected.

The current diagnostic frontier is therefore no longer a specific missing HLE. The next hardware run must classify the sustained black-screen path as either:

- an active translated/game/display loop; or
- a durable translated-thread stall.

`main` now includes an independent Horizon watchdog that records that distinction in `fast-track-heartbeat-history.txt`.

## Milestones

| Milestone | Status |
| --- | --- |
| Native Horizon/libnx runtime boots | ✅ Done |
| WiiCompiled PPC → AArch64 translated code executes on Switch | ✅ Done |
| Guest memory/data initialization | ✅ Done |
| HostContext / GuestFlat / translated handoff | ✅ Done |
| Reach Mario Kart Wii `main()` | ✅ Hardware validated |
| Enter post-`main` game initialization | ✅ Hardware validated |
| Thread/context continuation across guest `OSThread` switches | ✅ Hardware validated |
| Observed post-main OS/VI/WPAD/PAD/time/power/SC boundaries | ✅ Hardware validated |
| `OSWakeupThread` scheduler handoff | ✅ Hardware validated |
| Sustained post-main translated execution | ✅ Hardware validated |
| Classify active loop vs durable stall | 🟡 Current frontier |
| Game/resource initialization | 🟡 In progress |
| GX → Switch graphics backend / first frame | ⬜ Pending |
| Input/audio/filesystem completeness and gameplay | ⬜ Pending |

## Current fast-track path

```text
headless Horizon platform init
  ↓
PAL __start (0x800060A4)
  ↓
early Wii SDK / OS / NAND / DVD / VI bring-up
  ↓
PAL main() (0x8000B6B0)                    ✅ reached on hardware
  ↓
System::RKSystem::main / initialize        ✅ reached on hardware
  ↓
post-main MEM2 / mutex / thread / VI
  ↓
HostContext-backed guest OSThread switch   ✅ hardware validated
  ↓
WPAD / PAD initialization                  ✅ hardware crossed
  ↓
OSGetTime (0x801AAD5C)                     ✅ hardware crossed
  ↓
OSSetPowerCallback (0x801AB75C)            ✅ hardware crossed
  ↓
SCGetProductArea (0x801B23A0)              ✅ hardware crossed
  ↓
OSWakeupThread (0x801AAAA4)                ✅ hardware crossed
  ↓
sustained post-main translated execution   ✅ 37,148 total dispatches
  ↓
EGG AsyncDisplay range (0x8020FCD4...)      ✅ reached
  ↓
active-loop vs durable-stall classification ← current frontier
  ↓
resource / graphics bring-up
  ↓
first rendered frame
```

The complete blocker-by-blocker history and current checklist live in [`ROADMAP.md`](ROADMAP.md). Hardware evidence is recorded in dated files under [`docs/`](docs/).

## Important limitations

### Graphics

The GX FIFO bridge is still intentionally a sink. There is no real GX → Switch renderer yet, so a **black screen is expected** even while translated CPU execution is advancing correctly. First-frame work belongs to M3.

### Filesystem / DVD

The project does not fabricate Nintendo game data. A real local DVD/FST mapping still has to be published from the user's own dump when resource loading requires it.

### Input

Some pinned WPAD/PAD initialization boundaries are mirrored because hardware reached them, but full Joy-Con / Pro Controller / Wii Remote / GameCube input semantics are **not** implemented yet. Input behavior is added only when hardware evidence proves the required boundary and pinned semantics.

## Local fast-track hardware test

After pulling `main`:

```sh
git checkout main
git pull
git submodule update --init --recursive

MKW_JOBS=4 bash scripts/build-local-fast-track.sh
```

For repeat local rebuilds, the incremental helper also exists:

```sh
MKW_JOBS=4 bash scripts/build-local-fast-track-incremental.sh
```

Copy `WiiCompiled-Switch-local-fast-track.nro` to the Switch and launch it through hbmenu in application/title-override mode with full memory.

The fast-track is intentionally headless. Use the SD diagnostic files instead of expecting a text console:

```text
/switch/WiiCompiled-Switch/fast-track-progress.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
/switch/WiiCompiled-Switch/fast-track-heartbeat-history.txt
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

For the current sustained-black-screen frontier, `fast-track-heartbeat-history.txt` is the primary diagnostic. `ACTIVE` samples mean the translated heartbeat continues to change; consecutive `STALE` samples mean the independent Horizon watchdog is alive while translated dispatch has stopped advancing.

## Public CI boundary

Public CI remains Nintendo-data-free. A real WiiCompiled Mario Kart Wii product is generated locally from a user-owned dump before the Switch build and linked into the NRO. Generated game-derived C++/objects/data and game-containing NRO/ELF artifacts are never committed or uploaded by public CI.

The repository currently validates five CI workflows for fast-track changes:

- `lint`;
- `fast-track-startup`;
- `stateful-translated-sequence`;
- `bootstrap-register-prelude`;
- `build-switch`.

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, decrypted content, or generated translated game output**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See [`LEGAL.md`](LEGAL.md).

## Requirements

- A Nintendo Switch capable of running homebrew under Atmosphère
- Homebrew Menu (`hbmenu`)
- devkitPro with `devkitA64` and `libnx`
- GNU Make

Initialize the pinned WiiCompiled source and build the Nintendo-data-free runtime probe:

```sh
git submodule update --init --recursive
make
```

Expected public probe output:

```text
WiiCompiled-Switch.nro
```

The public probe contains no translated Mario Kart Wii product. Game-derived translation is a separate local-only build path.

## Current architecture

```text
user-owned Wii dump (local build only)
            |
            v
WiiCompiled translator
            |
            v
generated C++ / RuntimeConfig / data init
            |
            +------ linked at build time ------+
                                              |
                                              v
+----------------------------------------------------------+
| Native AArch64 NRO                                      |
| WiiCompiled translated product + runtime                |
| Switch platform adapter                                 |
| - lifecycle / applet                                    |
| - runtime filesystem + diagnostics / NAND backing       |
| - input HLE state (partial, hardware-driven)            |
| - audio (bootstrap/HLE incomplete)                      |
| - graphics (GX FIFO sink; renderer pending)             |
| - context switching / timing / guest memory             |
| - independent fast-track liveness watchdog              |
+----------------------------------------------------------+
            |
            v
        libnx / Horizon
            |
            v
       Atmosphère / Switch
```

## Roadmap and evidence

Start with:

- [`ROADMAP.md`](ROADMAP.md) — authoritative current milestone/frontier checklist;
- [`docs/M2_RUNTIME_BOOTSTRAP.md`](docs/M2_RUNTIME_BOOTSTRAP.md) — current runtime/bootstrap architecture and hardware method;
- [`docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`](docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md) — first real `main()` proof;
- [`docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md`](docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md) — HostContext guest continuation proof;
- [`docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md`](docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md) — scheduler frontier that preceded sustained execution;
- [`docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`](docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md) — current sustained black-screen / AsyncDisplay evidence.

Older dated `HARDWARE_RESULTS_*` files are historical snapshots. Their “next blocker” wording intentionally reflects what was known on that date and is not rewritten retroactively.

## Upstream

The audited WiiCompiled revision is pinned to:

```text
a135beb201042b20f390c6695ca6b26768820fb4
```

CI enforces the pin. New hardware blockers are mapped against that exact revision before any HLE behavior is added.
