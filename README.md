# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Project progress

**Estimated progress toward first rendered frame: ~25%**

```text
█████░░░░░░░░░░░░░ 25%
```

> This percentage is an engineering estimate, not a function-count metric. It tracks progress toward the first rendered Mario Kart Wii frame on real Switch hardware.

| Milestone | Status |
| --- | --- |
| Native Horizon/libnx runtime boots | ✅ Done |
| WiiCompiled PPC → AArch64 translated code executes on Switch | ✅ Done |
| Guest memory/data initialization | ✅ Done |
| Wii SDK OS/cache/timing/interrupt bootstrap | 🟡 In progress |
| EXI/SI bootstrap and basic EXI transaction HLE | 🟡 In progress |
| Reach Mario Kart Wii `main()` | ⬜ Next major milestone |
| Game/resource initialization | ⬜ Pending |
| GX → Switch graphics backend / first frame | ⬜ Pending |
| Input, audio, filesystem completeness and gameplay | ⬜ Pending |

Current execution path:

```text
__start
  ↓
Wii SDK / OS bootstrap
  ↓
cache / timing / interrupts
  ↓
EXI / SI initialization      ← current area
  ↓
__init_user
  ↓
main()                       ← next major target
  ↓
Mario Kart Wii initialization
  ↓
GX / resources / input
  ↓
first rendered frame
  ↓
playable game
```

## Status

**M2 — Fast-track translated startup toward `main()`.** The project now executes real WiiCompiled-translated Mario Kart Wii startup code as AArch64 under Horizon/libnx on real Switch hardware.

Validated hardware/runtime work includes GuestFlat memory, Wii `Memory::Init`, translated data initialization, translated `__start`, register bootstrap, timebase/SPR/FPSCR helpers, OS timing and interrupt state, exception/interrupt initialization, Wii cache-control HLE, EXI initialization/basic transactions, and SI initialization.

The latest real-hardware run reached `EXIImm` (`0x80167F68`). Its pinned WiiCompiled HLE semantics, together with `EXIDma`, `EXISync`, and `EXIUnlock`, are now implemented in `main`; the next hardware run is expected to advance beyond that boundary.

Public CI remains Nintendo-data-free. A real WiiCompiled game product is generated from a user-owned dump **before the Switch build** and linked into the same NRO. Generated game-derived code/data and local game NRO/ELF outputs are never committed or uploaded by CI.

Graphics and audio remain explicit stubs in M2. The project has **not yet proven entry into Mario Kart Wii `main()` or rendered a frame**.

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, decrypted content, or generated translated game output**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See `LEGAL.md`.

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

Expected output:

```text
WiiCompiled-Switch.nro
```

Copy it to:

```text
/switch/WiiCompiled-Switch/WiiCompiled-Switch.nro
```

Launch it from hbmenu in application/title-override mode with full memory rather than Album applet mode.

The public probe should report `translated product: NOT LINKED` and stop at `WAITING_FOR_TRANSLATED_PRODUCT`. That is expected and is not an error.

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
| - runtime filesystem                                    |
| - input                                                 |
| - audio (stubbed)                                       |
| - graphics (stubbed)                                    |
| - threading / timing                                    |
| - virtual memory                                        |
+----------------------------------------------------------+
            |
            v
        libnx / Horizon
            |
            v
       Atmosphère / Switch
```

## Roadmap

See `ROADMAP.md`, `docs/M2_RUNTIME_BOOTSTRAP.md`, and `docs/TRANSLATED_PRODUCT_BOUNDARY.md`.

## Upstream

The long-term aim is to keep Switch-specific changes narrow enough that they can eventually be proposed upstream to WiiCompiled rather than maintaining a permanent fork. The audited upstream revision is recorded in `UPSTREAM.md` and enforced by CI.
