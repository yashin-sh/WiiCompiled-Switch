# First private native Deko3D candidate — 2026-10-14

Code candidate [`e217a6f`](https://github.com/yashin-sh/WiiCompiled-Switch/commit/e217a6f9ed1a43a4c9e9031a15d5b6578ba1c0fb)
completes PAL translation, all 72 native game-code blocks, the full Release
product link and private NRO packaging. It is deployed to the Switch with
complete size/SHA-256 readback and verified removal of the older project NRO.
Console execution, boot/menu progression and race performance remain pending.

| Evidence | Result |
| --- | --- |
| Fresh PAL translation and 72 game-code blocks | Passed |
| Full product link and NACP/RomFS packaging | Passed |
| NRO segments and AArch64 ELF validation | Passed |
| Archived runtime/Aurora source audit | 1,206 files match the reference or the six authorized product changes |
| Complete SD NRO readback | Passed |
| Older project NRO backup against recorded size/SHA, removal and final inventory | Passed; one old candidate removed |
| Native configuration and verified-extraction marker readback | Passed |
| Real Switch launch, menu, Time Trial, 12-kart race | Pending |
| Published checks on the final PR HEAD | Tracked separately in [PR #387](https://github.com/yashin-sh/WiiCompiled-Switch/pull/387) |

## Exact artifact

```text
source commit: e217a6f9ed1a43a4c9e9031a15d5b6578ba1c0fb
NRO bytes:     74444944
NRO SHA-256:   82de60349a066efc0193e343907329b9d8dfea222f3bc5b8ed64fd0bfe6f7243
SD path:       sdmc:/switch/WiiCompiled-Switch-native-deko3d-e217a6f.nro
```

The final lint-only revision rebuilds/repackages successfully and produces the
same NRO bytes as the preceding successful product. The debug ELF, translated
code, raw build logs, game inputs and cleanup receipts remain local and ignored.

## Toolchain and repairs

The build uses the existing local `wiicompiled-m3-mesa-b297e230-v3` Docker image,
identified as
`sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`,
with devkitA64 GCC 15.2.0 and release `deko3d`. Dawn and UAM are freshly built
from the pins in the [native build guide](NATIVE_DEKO3D_SWITCH_PATH_2026-10-12.md).
The successful product uses six build workers and a four-worker translated-code
pool. Threaded GX, frame-encode overlap and function ordering are enabled; PGO
and Deko3D validation are off.

The first link exposed anonymous-namespace diagnostic C symbols, an OpenGL
adapter constructor despite the Deko3D-only archive, and missing SQLite
ownership syscalls. The checked product patcher exports those diagnostic symbols
globally, aligns backend flags, advertises Deko3D for automatic selection and
applies the three existing SQLite Switch fixes with complete source/patch/result
hashes. The pinned reference remains clean.

Tests pass for both unity inclusion orders, Switch Deko3D and Android Vulkan/GLES
selection, real SQLite create/write/reopen/query behavior, device-prefixed paths,
injected EOF/EIO and genuine I/O errors. Poisoned unsupported ownership symbols
verify that the SQLite binary does not depend on them. The eight existing
preparation/reuse contracts, Ruff 0.16.7, shellcheck and workflow lint also pass
locally. Every published final-HEAD GitHub check must still succeed before merge.

## SD preparation and next test

The existing PAL extraction is preserved. A read-only preflight verifies all
2,096 FST entries, including 2,032 files with exact declared sizes totaling
2,682,689,784 bytes. SD `main.dol` and `StaticR.rel` fully match the translated
local inputs. No full-content digest comparison is claimed for the other assets.

The native profile at `sdmc:/switch/WiiCompiled/Config.toml` points at
`../WiiCompiled-Switch/DATA`, selects Deko3D and enables threaded GX, frame-encode
overlap and the FPS display at resolution scale 1.0. The launcher's format-1
marker is created only after the extraction preflight; both small files pass
complete readback. Existing game files, NAND data and unrelated applications
are preserved. The old candidate is backed up and rehashed independently before
removal; the root Switch directory contains only the current project NRO.

Quit DBI, open hbmenu through application/title override with full memory, launch
**WiiCompiled-Switch Native Deko3D**, then select Mario Kart Wii in the launcher.
First validate the Wiimote page and actual game menu. Only then run a complete
stock-clock Time Trial and 12-kart race, retaining attributable timing and fresh
diagnostics. No playability or measured FPS is claimed by this build/deployment.
