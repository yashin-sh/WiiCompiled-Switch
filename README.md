# WiiCompiled-Switch

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Status

**Milestone 0 — platform bootstrap.** The repository currently builds a minimal libnx application and establishes the Switch platform boundary. The actual WiiCompiled runtime is not wired in yet.

## Legal / content policy

This repository contains **no Nintendo game code, ROM, disc image, keys, firmware, copyrighted game assets, or decrypted content**. Users must provide their own legally obtained game dump locally. Do not commit generated game data or extracted assets.

WiiCompiled is GPL-3.0; derivative code in this repository is therefore GPL-3.0 unless a file says otherwise. See `LEGAL.md`.

## Requirements

- A Nintendo Switch capable of running homebrew under Atmosphère
- Homebrew Menu (`hbmenu`)
- devkitPro with `devkitA64` and `libnx`
- GNU Make

On a devkitPro shell:

```sh
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

and launch it from hbmenu.

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

See `ROADMAP.md`.

## Upstream

The long-term aim is to keep Switch-specific changes narrow enough that they can eventually be proposed upstream to WiiCompiled rather than maintaining a permanent fork.
