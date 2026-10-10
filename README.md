# WiiCompiled-Switch

Experimental Nintendo Switch homebrew port of [WiiCompiled](https://github.com/patchzyy/Wiicompiled): statically recompile a user-owned Mario Kart Wii dump into a native AArch64 `.nro`, without Dolphin at runtime.

## Status

A recognizable Wiimote safety page is now captured on Switch and reproduced by desktop FIFO replay. Later checkpoints remain black; menu progression, steady performance and playability are unproven.
Real translated execution, observed boot-resource reads and the Aurora → Dawn/WebGPU → Vulkan/NVK presentation path run on Switch.
With captures disabled and diagnostic SD writes sampled, early startup reaches 7.5–9.3 Hz before later stalls. The capture-enabled trial retains 90 replayable frames. Subsequent capture-disabled runs accept the KD post-resume request and the projection getter. The scissor-origin, light-object, normal-matrix, observed depth-texture disable and matrix-30 texture-coordinate calls now return. The latest translated stop is an all-zero GXSetFog tuple; the preceding Dawn copy-source validation error is absent from the fresh graphics report.
The [latest image and replay evidence](docs/HARDWARE_NONBLACK_REPLAY_2026-10-09.md) records the recognizable boot page and black later checkpoint. The [fresh S10 trial](docs/HARDWARE_TEV_S10_DRAW_QUAD_2026-10-10.md) independently verifies 39 reports, accepts the observed material's three signed-color returns through checked caller progression, and reaches `DrawQuad` (`0x80084D20`). Captures are disabled; later game pixels remain unproven. The [new quad bridge](docs/LYT_DRAW_QUAD_2026-10-10.md) validates complete inputs/layout before native submission and passes packet contracts and fresh textured/colored GPU capture/replay tests; its private build and all eight exact-HEAD checks pass, [PR #352](https://github.com/yashin-sh/WiiCompiled-Switch/pull/352) is merged, and complete SD readback verifies the new NRO. The exact candidate transfers via nxlink with exit 0 at 2026-10-10 08:38:11 UTC; [Fresh USB reports](docs/HARDWARE_DRAW_QUAD_IA8_REPEAT_2026-10-10.md) accept the observed DrawQuad return and diagnose a 32 × 32 IA8/repeat texture refusal. The [IA8 correction](docs/GX_IA8_REPEAT_2026-10-10.md) forwards checked wrap modes and passes fresh GPU repeat/refresh/replay oracles; its private build and all eight published exact-HEAD checks pass, [PR #355](https://github.com/yashin-sh/WiiCompiled-Switch/pull/355) is merged, and the corrected NRO passes complete SD readback. The preceding trial still shows Wiimote, then black and a crash. The exact corrected IA8 NRO launches via nxlink with exit 0 at 2026-10-10 12:51:41 CEST (10:51:41 UTC); [Fresh USB reports](docs/HARDWARE_IA8_CLAMP_2026-10-10.md) verify 39 reports / 839,522 bytes and diagnose an IA8/clamp descriptor after another observed quad return. The [clamp correction](docs/GX_IA8_CLAMP_2026-10-10.md) passes both-mode contracts and fresh clamp/repeat GPU oracles; its private build and all eight published exact-HEAD checks pass, [PR #358](https://github.com/yashin-sh/WiiCompiled-Switch/pull/358) is merged, and the corrected NRO passes complete SD readback. Its exact NRO launches through nxlink with exit 0 at 2026-10-10 13:45:46 CEST; [fresh USB reports](docs/HARDWARE_IA4_REPEAT_2026-10-10.md) verify 39 reports / 841,601 bytes and reach an IA4/repeat guard after further layout progression. The [IA4 correction](docs/GX_IA4_2026-10-10.md) passes both-mode bridge contracts and fresh GPU/replay tests, with independent S/T oracles for IA4 and IA8. Its private build and all eight published exact-HEAD checks pass, [PR #360](https://github.com/yashin-sh/WiiCompiled-Switch/pull/360) is merged, and the corrected NRO passes complete SD readback. Its exact NRO launches through nxlink with exit 0 at 2026-10-10 14:42:15 CEST; the [fresh USB result](docs/HARDWARE_IA4_FOG_ZERO_2026-10-10.md) verifies 39 reports / 841,828 bytes and accepts the retained relocated IA4/repeat load before an all-zero Fog guard. The [Fog correction](docs/GX_FOG_ZERO_2026-10-10.md) admits only the third exact tuple and passes both-mode bridge, native BP and negative-control tests. Candidate `bb00520` passes its private build and all eight exact-HEAD checks; [PR #363](https://github.com/yashin-sh/WiiCompiled-Switch/pull/363) is merged and the new NRO passes complete SD readback. Corrected console execution remains pending.
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
