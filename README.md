# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Current status

The project now executes real WiiCompiled-translated Mario Kart Wii code on real Switch hardware and has **reached PAL `main()` (`0x8000B6B0`) after 605 translated dispatches**. Post-`main` execution is also proven through `System::RKSystem::main` (`0x80008EF0`) and `System::RKSystem::initialize` (`0x80009194`).

The current work is no longer “reach `main()`”. It is the **post-main initialization fast-track** tracked in issue #117: run on hardware, stop at the first unsupported native/translated boundary, mirror the exact pinned WiiCompiled semantics, validate in Nintendo-data-free CI, merge, and repeat.

Current hardware frontier on `main`:

- `WPADInit` (`0x801BF5C4`) — crossed on hardware;
- `WPADGetDpdSensitivity` (`0x801C329C`) — crossed on hardware;
- `WPADGetStatus` (`0x801BF64C`) — crossed on hardware;
- `WPADControlMotor` (`0x801C0EC4`) — crossed on hardware;
- `PADInit` (`0x801AF2F0`) — bridge merged; **next hardware run must prove the next boundary**.

The most recent `PADInit` bridge mirrors the pinned host contract only: initialization is idempotent and guest-visible success is `r3 = 1`. It does **not** pre-port `PADRead`, reset, recalibration, physical rumble, SDL controller objects, or Joy-Con/GameCube mappings.

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
| Post-main OS/VI/WPAD/PAD initialization | 🟡 In progress |
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
PAL main() (0x8000B6B0)                  ✅ reached on hardware
  ↓
System::RKSystem::main / initialize      ✅ reached on hardware
  ↓
post-main MEM2 / mutex / thread / VI
  ↓
HostContext-backed guest OSThread switch ✅ hardware validated
  ↓
WPADInit
  ↓
WPADGetDpdSensitivity
  ↓
WPADGetStatus
  ↓
WPADControlMotor                         ✅ all crossed on hardware
  ↓
PADInit (0x801AF2F0)                     ← current merged frontier
  ↓
next hardware-proven post-main boundary
  ↓
resource / input / graphics bring-up
  ↓
first rendered frame
```

The complete blocker-by-blocker history and current checklist live in [`ROADMAP.md`](ROADMAP.md). Hardware evidence is recorded in dated files under [`docs/`](docs/), including the current `PADInit` result.

## Important limitations

### Graphics

The GX FIFO bridge is still intentionally a sink. There is no real GX → Switch renderer yet, so a **black screen is expected** even while translated CPU execution is advancing correctly. First-frame work belongs to M3.

### Filesystem / DVD

The project does not fabricate Nintendo game data. A real local DVD/FST mapping still has to be published from the user's own dump when resource loading requires it.

### Input

Some pinned WPAD/PAD initialization boundaries are now mirrored because hardware reached them, but full Joy-Con / Pro Controller / Wii Remote / GameCube input semantics are **not** implemented yet. Input behavior is added only when hardware evidence proves the required boundary and pinned semantics.

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
/switch/WiiCompiled-Switch/fast-track-main-reached.txt
/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
/switch/WiiCompiled-Switch/fast-track-exception.txt
```

`fast-track-main-reached.txt` records the already-proven PAL `main` milestone; the current loop normally advances through `fast-track-dispatch-blocker.txt`.

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
| - input HLE state (partial, hardware-driven)             |
| - audio (bootstrap/HLE incomplete)                      |
| - graphics (GX FIFO sink; renderer pending)             |
| - context switching / timing / guest memory             |
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
- [`docs/HARDWARE_RESULTS_2026-09-16_PAD_INIT.md`](docs/HARDWARE_RESULTS_2026-09-16_PAD_INIT.md) — current hardware frontier evidence.

Older dated `HARDWARE_RESULTS_*` files are historical snapshots. Their “next blocker” wording intentionally reflects what was known on that date and is not rewritten retroactively.

## Upstream

The audited WiiCompiled revision is pinned to:

```text
a135beb201042b20f390c6695ca6b26768820fb4
```

CI enforces the pin. New hardware blockers are mapped against that exact revision before any HLE behavior is added.
