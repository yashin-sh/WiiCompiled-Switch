# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Status

**M2 — Horizon runtime bootstrap.** Guest virtual memory, heap-backed checked GuestFlat, Wii `Memory::Init`, and the custom AArch64 cooperative-context primitive have hardware evidence in the M2 reports. The next integration slice now consumes the audited WiiCompiled source as a pinned submodule, compiles its SDL-free `RuntimePlatform` host layer into the NRO, implements the real upstream `HostContext` API on Horizon, and adds native libnx lifecycle/filesystem/timing/HID services.

Graphics and audio remain explicit stubs in this milestone. The runtime bootstrap intentionally stops at `WAITING_FOR_USER_DATA`; it does not yet claim translated Mario Kart Wii execution or rendering.

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, or decrypted content**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See `LEGAL.md`.

## Requirements

- A Nintendo Switch capable of running homebrew under Atmosphère
- Homebrew Menu (`hbmenu`)
- devkitPro with `devkitA64` and `libnx`
- GNU Make

Initialize the pinned WiiCompiled source and build:

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

## Current architecture

```text
WiiCompiled translated game/runtime
            |
            v
+------------------------------+
| Switch platform adapter      |
| - lifecycle / applet         |
| - filesystem                 |
| - input                      |
| - audio                      |
| - graphics                   |
| - threading / timing         |
| - virtual memory             |
+------------------------------+
            |
            v
        libnx / Horizon
            |
            v
       Atmosphère / Switch
```

## Roadmap

See `ROADMAP.md` and `docs/M2_RUNTIME_BOOTSTRAP.md`.

## Upstream

The long-term aim is to keep Switch-specific changes narrow enough that they can eventually be proposed upstream to WiiCompiled rather than maintaining a permanent fork. The audited upstream revision is recorded in `UPSTREAM.md` and enforced by CI.
