# WiiCompiled-Switch

Experimental Nintendo Switch homebrew port of [WiiCompiled](https://github.com/patchzyy/Wiicompiled): statically recompile a user-owned Mario Kart Wii dump into a native AArch64 `.nro`, without Dolphin at runtime.

## Status

A recognizable Wiimote safety page is now captured on Switch and reproduced by desktop FIFO replay. Later checkpoints remain black; menu progression, steady performance and playability are unproven.
Real translated execution, observed boot-resource reads and the Aurora → Dawn/WebGPU → Vulkan/NVK presentation path run on Switch.
With captures disabled and diagnostic SD writes sampled, early startup reaches 7.5–9.3 Hz before later stalls. The capture-enabled trial retains 90 replayable frames. Subsequent capture-disabled runs accept the KD post-resume request and the projection getter. The scissor-origin, light-object, normal-matrix and observed depth-texture disable calls now return. The latest translated stop is texture-coordinate generation with matrix 30; the same run also reports a Dawn texture-copy usage error.
The [latest image and replay evidence](docs/HARDWARE_NONBLACK_REPLAY_2026-10-09.md) records the recognizable boot page and black later checkpoint; the [latest progress report](docs/HARDWARE_Z_TEXTURE_TEX_COORD_2026-10-09.md) accepts depth-texture disable return and records both outstanding boundaries.
See [the roadmap](ROADMAP.md) for the current frontier and [the status log](docs/STATUS_LOG.md) for dated evidence.

## Quick build

Requires devkitPro (`devkitA64`, `libnx`), GNU Make and a Switch running Atmosphère/hbmenu. Local rendered builds additionally use Docker and locally prepared game data; follow [the build guide](docs/BUILD.md) before running them.

```sh
git submodule update --init --recursive
make                                           # Public, Nintendo-data-free probe
MKW_JOBS=4 bash scripts/build-local-rendered-fast-track.sh  # Local rendered product
```

The public output is `WiiCompiled-Switch.nro`; the local rendered output is `WiiCompiled-Switch-local-rendered-fast-track.nro`. Launch through hbmenu in application/title-override mode with full memory. The [build guide](docs/BUILD.md) covers the headless control, incremental and Discovery targets, diagnostics and required checks.

## Limits

- Graphics: recognizable boot-screen pixels and desktop replay are proven; later rendering and complete GX behavior remain under validation.
- DVD/filesystem: real FST and boot-resource reads work on the observed path; broader semantics are incomplete.
- IOS/network: only observed boot-time KD request sequences are supported; online play is unimplemented.
- Input: partial controller support; complete mapping and per-button hardware validation remain open.
- Audio: native audio output is not hardware-proven.

## Public source and CI

Provide your own legally obtained game dump locally. Keep game data, generated translations, private NROs and raw captures out of Git and public CI. Public CI validates Nintendo-data-free code and probes; game-bound renderer changes also require a private rendered build and attributable hardware evidence. Every published PR check must be green before merging; `main` enforces eight required checks. See [the merge policy](docs/CI_MERGE_POLICY_2026-10-09.md), [validation policy](docs/FAST_TRACK_VALIDATION_POLICY.md) and [legal policy](LEGAL.md). WiiCompiled and derivative code are GPL-3.0 unless stated otherwise.

[Roadmap](ROADMAP.md) · [Documentation](docs/README.md) · [Report terminology](docs/REPORT_TERMS.md) · [Build and diagnostics](docs/BUILD.md) · [Desktop replay prototype](docs/DESKTOP_GX_REPLAY.md) · [Status history](docs/STATUS_LOG.md)
