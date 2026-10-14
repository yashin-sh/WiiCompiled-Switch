# WiiCompiled-Switch

Experimental Nintendo Switch homebrew port of [WiiCompiled](https://github.com/patchzyy/Wiicompiled): statically recompile a user-owned Mario Kart Wii dump into a native AArch64 `.nro`, without Dolphin at runtime.

## Current checkpoint — 2026-10-14

The experimental native Deko3D build path in [PR #386](https://github.com/yashin-sh/WiiCompiled-Switch/pull/386)
passes all **10 published checks** on source commit
[`42aca1c`](https://github.com/yashin-sh/WiiCompiled-Switch/commit/42aca1c1b9d9667185cc9cd1b2d5f5a971419958).
The [public dependency build](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/38574319340)
produces pinned UAM and Dawn/Deko3D static archives and verifies their symbols,
configuration and provenance. Documentation-only follow-ups must pass their own exact-HEAD CI.

| Path | Verified here | Still required |
| --- | --- | --- |
| Native Deko3D candidate | Public dependency cross-build, full private PAL product build, exact NRO attribution and verified SD deployment | Switch boot/menu execution, Time Trial and 12-kart race validation |
| Existing Vulkan/NVK diagnostic path | Earlier Switch boot pixels, translated startup and desktop FIFO replay | Later rendering, complete menu/race behavior and performance validation |

The first private [Deko3D candidate](docs/NATIVE_DEKO3D_CANDIDATE_2026-10-14.md),
code `e217a6f`, now passes full PAL translation/product build, artifact/provenance
validation and complete SD readback with verified older-NRO cleanup. It is ready
for the first console trial. Menu progression, playability and race FPS remain unproven.
The user clarified on 2026-10-14 that private builds do not block merging into
`main`: every published check on the final PR HEAD must be completed and successful.
Private product builds and Switch measurements remain separate readiness evidence.
The NVK diagnostic path remains available.

To prepare the private Deko3D candidate with your own PAL `main.dol` and `StaticR.rel`
in `local-product/Assets`, follow the prerequisites and receipts in the
[native Deko3D guide](docs/NATIVE_DEKO3D_SWITCH_PATH_2026-10-12.md):

```bash
bash scripts/build-local-native-deko3d.sh
```

The console launcher imports your own PAL ISO/WBFS or uses a verified extracted
DATA tree. The private NRO embeds locally generated program/data and stays Git-ignored;
game resources remain on SD.
See [the status log](docs/STATUS_LOG.md) for dated evidence and remaining gates.

## Earlier NVK checkpoints

Selector candidate `4e5087d` passes its exact-source private rendered gate,
all eight published exact-HEAD checks and verified LAN deployment. Fresh console
reports advance beyond the color-selector frontier, record the alpha-selection
stage and pause at
`GXSetTevIndirect` (`0x801717AC`), dispatch 659,106. See the [KSEL result](docs/GX_TEV_KSEL_2026-10-11.md)
and [next bounded indirect correction](docs/GX_TEV_INDIRECT_2026-10-11.md).
The next candidate's build, CI, deployment and hardware gates remain pending;
automatic NRO restart and later game pixels are unproven.

The NANDGetLength candidate passes its private rendered/Discovery gate and all eight exact-HEAD GitHub checks; PR #383 is merged. Complete SD readback and verified older-NRO cleanup pass. Corrected Netloader execution remains pending. See the [candidate receipt](docs/NAND_GET_LENGTH_2026-10-11.md).


The NANDSeek candidate launches successfully, but the verified trial stops earlier at NANDGetLength on the existing-file branch. No rewind execution is accepted. See the [hardware result](docs/HARDWARE_NAND_SEEK_LENGTH_2026-10-11.md) and [bounded length correction](docs/NAND_GET_LENGTH_2026-10-11.md).


The NANDSeek candidate passes its private rendered/Discovery gate and all eight exact-HEAD GitHub checks; PR #381 is merged. Complete SD readback and verified older-NRO cleanup pass. The subsequent Netloader trial is verified and reaches the earlier existing-file NANDGetLength branch. See the [candidate receipt](docs/NAND_SEEK_2026-10-11.md).


The [fresh NANDMove result](docs/HARDWARE_NAND_MOVE_SEEK_2026-10-11.md) verifies 42 reports twice, accepts the actual move with result 0 and independent destination/source checks, and reaches NANDSeek `0x8019B964` with offset/origin 0. The operator still sees Wiimote, then black output and an error. The [bounded rewind correction](docs/NAND_SEEK_2026-10-11.md) validates a live owned ordinary write handle and performs the actual pinned seek/tell sequence; corrected console execution remains pending.


The NANDMove candidate passes its private rendered/Discovery gate and all eight exact-HEAD GitHub checks; PR #378 is merged. Complete SD readback and verified older-NRO cleanup pass. The exact NRO launches via nxlink with exit 0 at 2026-10-11 10:38:33 CEST; durable USB results remain pending. See the [candidate receipt](docs/NAND_MOVE_2026-10-10.md).


The [fresh NAND save result](docs/HARDWARE_NAND_WRITE_MOVE_2026-10-10.md) verifies 41 reports / 853,191 bytes twice, accepts the complete 29,344-byte write and close/home-query progression, and reaches NANDMove `0x8019BEE8`. The previous candidate is replaced on SD with complete readback and verified older-NRO cleanup. The [bounded move correction](docs/NAND_MOVE_2026-10-10.md) checks both full live paths and preserves pinned destination-directory semantics; its exact candidate launches on 2026-10-11 and durable console results remain pending.


## NVK diagnostic status

An opt-in [LAN debug server](docs/DEBUG_SERVER.md) provides live dispatch/status
logs, report downloads and verified NRO deployment/restart requests. Enable it
with `Config/debug.ini` only for debug runs; Switch hardware validation remains
separate from protocol and build checks.
Program restart is unsupported in the next correction: hbmenu title override
can restart its retail host game. The coordinated hbloader route and its remaining
validation are documented in the [debug-server guide](docs/DEBUG_SERVER.md).

A recognizable Wiimote safety page is now captured on Switch and reproduced by desktop FIFO replay. Later checkpoints remain black; menu progression, steady performance and playability are unproven.
Real translated execution, observed boot-resource reads and the Aurora → Dawn/WebGPU → Vulkan/NVK presentation path run on Switch.
With captures disabled and diagnostic SD writes sampled, early startup reaches 7.5–9.3 Hz before later stalls. The capture-enabled trial retains 90 replayable frames. Subsequent capture-disabled runs accept the KD post-resume request and the projection getter. The scissor-origin, light-object, normal-matrix, observed depth-texture disable and matrix-30 texture-coordinate calls now return. The current LAN-debug trial accepts the observed RGB5A3/repeat texture load and progression beyond the K color frontier to the alpha-selector stage, then reaches GXSetTevIndirect; see the [verified texture return](docs/GX_RGB5A3_REPEAT_2026-10-11.md), [selector result](docs/GX_TEV_KSEL_2026-10-11.md) and [next bounded correction](docs/GX_TEV_INDIRECT_2026-10-11.md). The earlier NANDSeek trial stopped at NANDGetLength on the existing-file read branch; the preceding Dawn copy-source validation error is absent from the fresh graphics report.
The [latest image and replay evidence](docs/HARDWARE_NONBLACK_REPLAY_2026-10-09.md) records the recognizable boot page and black later checkpoint. The [fresh S10 trial](docs/HARDWARE_TEV_S10_DRAW_QUAD_2026-10-10.md) independently verifies 39 reports, accepts the observed material's three signed-color returns through checked caller progression, and reaches `DrawQuad` (`0x80084D20`). Captures are disabled; later game pixels remain unproven. The [new quad bridge](docs/LYT_DRAW_QUAD_2026-10-10.md) validates complete inputs/layout before native submission and passes packet contracts and fresh textured/colored GPU capture/replay tests; its private build and all eight exact-HEAD checks pass, [PR #352](https://github.com/yashin-sh/WiiCompiled-Switch/pull/352) is merged, and complete SD readback verifies the new NRO. The exact candidate transfers via nxlink with exit 0 at 2026-10-10 08:38:11 UTC; [Fresh USB reports](docs/HARDWARE_DRAW_QUAD_IA8_REPEAT_2026-10-10.md) accept the observed DrawQuad return and diagnose a 32 × 32 IA8/repeat texture refusal. The [IA8 correction](docs/GX_IA8_REPEAT_2026-10-10.md) forwards checked wrap modes and passes fresh GPU repeat/refresh/replay oracles; its private build and all eight published exact-HEAD checks pass, [PR #355](https://github.com/yashin-sh/WiiCompiled-Switch/pull/355) is merged, and the corrected NRO passes complete SD readback. The preceding trial still shows Wiimote, then black and a crash. The exact corrected IA8 NRO launches via nxlink with exit 0 at 2026-10-10 12:51:41 CEST (10:51:41 UTC); [Fresh USB reports](docs/HARDWARE_IA8_CLAMP_2026-10-10.md) verify 39 reports / 839,522 bytes and diagnose an IA8/clamp descriptor after another observed quad return. The [clamp correction](docs/GX_IA8_CLAMP_2026-10-10.md) passes both-mode contracts and fresh clamp/repeat GPU oracles; its private build and all eight published exact-HEAD checks pass, [PR #358](https://github.com/yashin-sh/WiiCompiled-Switch/pull/358) is merged, and the corrected NRO passes complete SD readback. Its exact NRO launches through nxlink with exit 0 at 2026-10-10 13:45:46 CEST; [fresh USB reports](docs/HARDWARE_IA4_REPEAT_2026-10-10.md) verify 39 reports / 841,601 bytes and reach an IA4/repeat guard after further layout progression. The [IA4 correction](docs/GX_IA4_2026-10-10.md) passes both-mode bridge contracts and fresh GPU/replay tests, with independent S/T oracles for IA4 and IA8. Its private build and all eight published exact-HEAD checks pass, [PR #360](https://github.com/yashin-sh/WiiCompiled-Switch/pull/360) is merged, and the corrected NRO passes complete SD readback. Its exact NRO launches through nxlink with exit 0 at 2026-10-10 14:42:15 CEST; the [fresh USB result](docs/HARDWARE_IA4_FOG_ZERO_2026-10-10.md) verifies 39 reports / 841,828 bytes and accepts the retained relocated IA4/repeat load before an all-zero Fog guard. The [Fog correction](docs/GX_FOG_ZERO_2026-10-10.md) admits only the third exact tuple and passes both-mode bridge, native BP and negative-control tests. Candidate `bb00520` passes its private build and all eight exact-HEAD checks; [PR #363](https://github.com/yashin-sh/WiiCompiled-Switch/pull/363) is merged and the new NRO passes complete SD readback. The exact corrected NRO launches via nxlink with exit 0 at 2026-10-10 15:33:09 CEST; the [fresh result](docs/HARDWARE_FOG_IA4_LARGE_2026-10-10.md) verifies 39 reports / 842,870 bytes and accepts one Fog-zero return through checked caller progression before an IA4/clamp 1024-square descriptor guard. The [correction](docs/GX_IA4_LARGE_2026-10-10.md) validates the pinned 15-bit count encoding while requiring the full 1 MiB backing. Both-mode contracts, four negative controls and the fresh large GPU/replay scene pass; candidate `5549ab5` passes its private build and all eight exact-HEAD checks. PR #366 is merged and the corrected NRO passes complete SD readback. The exact corrected NRO launches via nxlink with exit 0 at 2026-10-10 16:17:10 CEST; the [fresh result](docs/HARDWARE_IA4_LARGE_NAND_CHECK_2026-10-10.md) verifies 39 reports / 850,130 bytes and accepts the retained 1024-square IA4/clamp load before a NANDCheck stop on the save-worker fiber. The user reports black output. The [bounded correction](docs/NAND_CHECK_2026-10-10.md) passes 562 production trait/Memory/pinned-oracle cases in each mode and rejects five incorrect variants. Candidate `b848972` passes the separate exact-source private rendered-build gate and all eight published checks in the complete paginated rollup. [PR #369](https://github.com/yashin-sh/WiiCompiled-Switch/pull/369) merges at 2026-10-10T15:11:35Z as `50618fd`. The 74,436,664-byte NRO passes full SD byte/SHA readback; the capture-disabled marker is independently reread. Every older owned project NRO is backed up, size/SHA verified and removed. The exact corrected NRO launches via nxlink with exit 0 at 2026-10-10 17:18:59 CEST. The [NANDCheck result](docs/HARDWARE_NAND_CHECK_CREATE_2026-10-10.md) verifies 39 reports / 851,184 bytes and accepts one observed return through checked caller progression. The next terminal is NANDCreate `0x8019B43C`, permission `0x30`, attributes 0. Original DOL attribution identifies `/tmp/banner.bin`; live bytes remain to be verified by the [bounded creation bridge](docs/NAND_CREATE_2026-10-10.md). Candidate `86b2ce5` passes the separate exact-source private rendered-build gate and all eight published checks in the complete paginated rollup. [PR #372](https://github.com/yashin-sh/WiiCompiled-Switch/pull/372) merges at 2026-10-10T16:04:08Z as `1ae1a11`. The 74,436,664-byte NRO passes complete SD byte/SHA readback. The capture-disabled marker is independently reread. Every older owned project NRO is backed up, size/SHA verified and removed, leaving only the current project candidate. The exact corrected NRO launches via nxlink with exit 0 at 2026-10-10 19:39:47 CEST; The [NANDCreate result](docs/HARDWARE_NAND_CREATE_WRITE_2026-10-10.md) verifies 40 reports / 851,403 bytes and accepts the observed creation with result 0 and matched live path. The next stop is NANDWrite `0x8019B884`, length `0x72A0`, after a successful temporary-file open. The [bounded write bridge](docs/NAND_WRITE_2026-10-10.md) validates the live buffer and owned handle before writing. Candidate `06fe994` passes the separate exact-source private rendered-build gate and all eight published checks in the complete paginated rollup. [PR #375](https://github.com/yashin-sh/WiiCompiled-Switch/pull/375) merges at 2026-10-10T18:18:46Z as `2b3652e`. The 74,440,760-byte NRO passes complete SD byte/SHA readback. The capture-disabled marker is independently reread. Every older owned project NRO is backed up, size/SHA verified and removed, leaving only the current project candidate. Corrected console execution remains pending.
The [save/NAND pre-port](docs/NAND_SAVE_PR_371_2026-10-10.md) repairs PR #371 against current main, preserves the bounded create/write guards and adds real filesystem/callback error contracts. PR #371 is merged after the private rendered build and all eight exact-HEAD checks pass. The exact new NRO launches directly via Netloader with exit 0; durable results and later SD cleanup remain pending. Its broader save entry points remain unproven on console.

See [the roadmap](ROADMAP.md) for the current frontier and [the status log](docs/STATUS_LOG.md) for dated evidence.

## Quick build

Requires devkitPro (`devkitA64`, `libnx`), GNU Make and a Switch running Atmosphère/hbmenu. Local rendered builds additionally use Docker and locally prepared game data; follow [the build guide](docs/BUILD.md) before running them.

```sh
git submodule update --init --recursive
make                                           # Public, Nintendo-data-free probe
MKW_JOBS=4 bash scripts/build-local-rendered-fast-track.sh  # Local NVK rendered product
bash scripts/build-local-native-deko3d.sh                  # Experimental private Deko3D product
```

The public output is `WiiCompiled-Switch.nro`; the local rendered output is `WiiCompiled-Switch-local-rendered-fast-track.nro`. Launch through hbmenu in application/title-override mode with full memory. The [build guide](docs/BUILD.md) covers the headless control, incremental and Discovery targets, diagnostics and required checks.

## Limits

- Graphics: recognizable boot-screen pixels and desktop replay are proven; later rendering and complete GX behavior remain under validation.
- DVD/filesystem: real FST and boot-resource reads work on the observed path; broader semantics are incomplete.
- IOS/network: only observed boot-time KD request sequences are supported; online play is unimplemented.
- Input: partial controller support; complete mapping and per-button hardware validation remain open.
- Audio: native audio output is not hardware-proven.

## Public source and CI

Provide your own legally obtained game dump locally. Keep game data, generated translations, private NROs and raw captures out of Git and public CI. Public CI validates Nintendo-data-free code and probes. Every published PR check must be green before merging; private builds and attributable hardware evidence validate product readiness and playability separately and do not block merge; `main` enforces eight required checks. See [the merge policy](docs/CI_MERGE_POLICY_2026-10-09.md), [validation policy](docs/FAST_TRACK_VALIDATION_POLICY.md) and [legal policy](LEGAL.md). WiiCompiled and derivative code are GPL-3.0 unless stated otherwise.

[Roadmap](ROADMAP.md) · [Documentation](docs/README.md) · [Report terminology](docs/REPORT_TERMS.md) · [Build and diagnostics](docs/BUILD.md) · [Desktop replay prototype](docs/DESKTOP_GX_REPLAY.md) · [Status history](docs/STATUS_LOG.md)
