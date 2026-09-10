# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Status

**M2 — Horizon runtime bootstrap.** Guest virtual memory, heap-backed checked GuestFlat, Wii `Memory::Init`, the custom AArch64 cooperative-context primitive, and the SDL-free Horizon lifecycle/filesystem/timing/HID bootstrap have real-Switch hardware evidence.

The current integration slice adds an explicit **build-time translated-product boundary**. Public CI builds contain only a Nintendo-data-free weak product stub, so after the validated runtime core initializes they intentionally stop at `WAITING_FOR_TRANSLATED_PRODUCT`.

A real WiiCompiled game product is generated from a user-owned dump **before the Switch build** and linked into the same NRO. The runtime does not load translated game code from an arbitrary SD-card `game-data` directory. SD-card paths are reserved for runtime state such as logs, cache, configuration and NAND/save-compatible data.

Graphics and audio remain explicit stubs in M2. No translated Mario Kart Wii entry point is executed yet.

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
