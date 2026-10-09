# WiiCompiled-Switch

Experimental Nintendo Switch homebrew port of [WiiCompiled](https://github.com/patchzyy/Wiicompiled): statically recompile a user-owned Mario Kart Wii dump into a native AArch64 `.nro`, without Dolphin at runtime.

## Status

An operator reports the Wiimote warning page, then black. With captures disabled and diagnostic SD writes sampled, early startup reaches 7.5–9.3 Hz before later stalls; recognizable captured game content, steady performance and playability are unproven.
Real translated execution, observed boot-resource reads and the Aurora → Dawn/WebGPU → Vulkan/NVK presentation path run on Switch.
The descriptor-based loader advances beyond the former Mii texture guard. The latest run stops at a KD request after scheduler resume. The next candidate mirrors its pinned phase and live handle lifetime; console acceptance remains pending.
The [latest console evidence](docs/HARDWARE_KD_POST_RESUME_2026-10-09.md) records the measured windows and exact KD stop; the [previous capture trial](docs/HARDWARE_CAPTURE_CONTROL_2026-10-09.md) records black checkpoints and a FIFO budget failure.
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

- Graphics: successful presents are proven; game pixels, later resources and complete GX behavior are still under validation.
- DVD/filesystem: real FST and boot-resource reads work on the observed path; broader semantics are incomplete.
- IOS/network: only observed boot-time KD request sequences are supported; online play is unimplemented.
- Input: partial controller support; complete mapping and per-button hardware validation remain open.
- Audio: native audio output is not hardware-proven.

## Public source and CI

Provide your own legally obtained game dump locally. Keep game data, generated translations, private NROs and raw captures out of Git and public CI. Public CI validates Nintendo-data-free code and probes; game-bound renderer changes also require a private rendered build and attributable hardware evidence. See [the validation policy](docs/FAST_TRACK_VALIDATION_POLICY.md) and [legal policy](LEGAL.md). WiiCompiled and derivative code are GPL-3.0 unless stated otherwise.

[Roadmap](ROADMAP.md) · [Documentation](docs/README.md) · [Report terminology](docs/REPORT_TERMS.md) · [Build and diagnostics](docs/BUILD.md) · [Desktop replay prototype](docs/DESKTOP_GX_REPLAY.md) · [Status history](docs/STATUS_LOG.md)
