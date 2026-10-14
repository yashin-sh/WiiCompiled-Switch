# First private native Deko3D candidate — 2026-10-14

Code candidate [`e217a6f`](https://github.com/yashin-sh/WiiCompiled-Switch/commit/e217a6f9ed1a43a4c9e9031a15d5b6578ba1c0fb)
completes PAL translation, all 72 native game-code blocks, the full Release
product link and private NRO packaging. It is deployed to the Switch with
complete size/SHA-256 readback and verified removal of the older project NRO.
The operator subsequently reports reaching character selection, where visual
bugs begin and the application then crashes. Native logs and the latest
Atmosphere report are retrieved and verified by independent reads; the console
argv and complete SD NRO digest match `e217a6f`. The report exposes a secondary
fault in the crash logger, obscuring the initial fault. Race completion and
performance remain unvalidated.

| Evidence | Result |
| --- | --- |
| Fresh PAL translation and 72 game-code blocks | Passed |
| Full product link and NACP/RomFS packaging | Passed |
| NRO segments and AArch64 ELF validation | Passed |
| Archived runtime/Aurora source audit | 1,206 files match the reference or the six authorized product changes |
| Complete SD NRO readback | Passed |
| Older project NRO backup against recorded size/SHA, removal and final inventory | Passed; one old candidate removed |
| Native configuration and verified-extraction marker readback | Passed |
| Real Switch launch and menu progression | Operator reports reaching character selection, then visual corruption and crash |
| Complete Time Trial and 12-kart race | Not validated; current candidate fails before a confirmed race |
| Original native diagnostics | Stable readbacks, exact argv/NRO match; secondary crash-handler fault symbolized |
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

## Follow-up after the character-selection crash

An independent audit identifies three defects in the pinned Dawn/Deko3D runtime:
texture uploads copy caller padding beyond the final required row; changing
shaders with the same WebGPU pipeline layout can preserve the previous shader's
combined texture/sampler mapping; descriptor slots can be overwritten while
older submitted GPU commands still reference them. These defects are not yet
established as the cause of the reported console crash.

The audited runtime repair repacks only copied texel blocks, reapplies bindings
on shader changes, and retires image/sampler descriptors behind the submission
fence. Texture binding invalidates the descriptor cache when slots are reused.
The complete three runtime files must match the pinned original or the exact
repair; all other base-patch hunks retain the reverse-apply verification.

The host regression compiles the production methods and reproduces the original
heap-buffer-overflow under AddressSanitizer. The repaired method passes with
minimum-size input buffers, padded rows/images, array layers, 3D slices, BC
formats, stencil and injected upload-allocation failure. Separate contracts
cover deferred descriptor reuse and shared-layout shader changes. These tests
do not emulate the Switch GPU. The corrected backend and full private NRO now
build and pass artifact/source/dependency validation. A new console trial
remains necessary before claiming a fix for character selection.

```text
runtime-fix build source: ddc2cd0a7f742e6c3482b4598d70a8ecc29cbc09
NRO bytes:               74444944
NRO SHA-256:             d52c76be1f0dc7a39d93452d51d7002aae8056b327513d02811abcf4935983d1
```

This intermediate GPU-runtime build is superseded by the combined runtime and
exception-stack candidate below. The preceding
NRO and debug ELF are retained locally and match their recorded complete
size/SHA-256, allowing fresh crash addresses to be resolved against that build.

The [Deko3D transfer implementation](https://github.com/devkitPro/deko3d/blob/master/source/dk_image.cpp)
uses byte row and image strides for `DkCopyBuf`, including compressed texel
blocks; the repair preserves those units while packing the upload.

## Retrieved native failure diagnostics

The retained console transcript identifies the deployed `e217a6f` NRO, whose
complete MTP readback matches its recorded 74,444,944-byte size and SHA-256.
The native stdout/stderr and recent Atmosphere crash report are stable across
two independent reads. Deko3D initializes and its 21 GPU smoke tests pass; the
game advances through multiple scenes. This does not validate all GX behavior,
menu correctness, a complete race or measured FPS.

Symbolization against the exact retained ELF places the repeated exception
frames in `SwitchCrashWrite`, `__libnx_exception_handler`, `_vfprintf_r`,
`__sbprintf` and `_fflush_r`. The exception logger faults recursively before its
first complete diagnostic reaches the SD card. The ELF confirms that the
runtime uses libnx's weak **0x400-byte exception stack**. The inference is that
newlib's temporary stdio buffer overruns that stack; the original game's fault
is not recovered from this secondary report.

The product repair supplies a strong, aligned 64 KiB exception stack and its
size symbol, matching the [libnx override contract](https://github.com/switchbrew/libnx/blob/v4.12.0/nx/source/runtime/init.c).
It also queries memory permissions and full FP/LR bounds before reading a stack
frame, preventing a corrupt frame chain from causing another fault. The host
contract compiles the complete repaired handler, links against simulated weak
libnx defaults, verifies the strong stack override and exercises unreadable,
out-of-range, truncated, cyclic and valid frame chains. The next console trial
must establish the initial fault or demonstrate corrected progression.

## Combined corrected candidate deployed

```text
source commit: 0b4f3c6fbe02da1cd8c6fd9b5f03f1c24017afdb
NRO bytes:     74444944
NRO SHA-256:   556ad379cb9e265277cfd5f498122a18f539527d38bf1b46cca226719c4ad657
SD path:       sdmc:/switch/WiiCompiled-Switch-native-deko3d-0b4f3c6.nro
```

The full private product rebuild and NRO/ELF/NACP/RomFS/source/dependency
validation pass. The ELF exposes a strong 65,536-byte exception-stack symbol
and a strong size symbol; the archive audit covers seven authorized product
files plus the three complete Dawn runtime files. The eight preparation/reuse
contracts and the production exception-handler and GPU-memory/binding contracts
pass. CI is tracked independently on the latest published PR HEAD.

Deployment completes at 2026-10-14 21:35:47 UTC. Complete SD size/SHA-256 readback
matches the new NRO. The old `e217a6f` candidate is copied locally, verified
against its recorded size/SHA, then removed. An independent directory read
confirms that only the current project's NRO remains. Before/after readbacks
verify the existing native configuration and extraction marker are unchanged;
unrelated entries and game/NAND data are preserved. Cleanup receipts remain
private and ignored. The new candidate has **not yet been run on the Switch**;
character selection, a complete Time Trial and a 12-kart race remain required.

## GPU depth readback repair

The next audit identifies a fourth independent GPU-runtime defect. Aurora's
`depth_peek.cpp` dispatches a compute shader into a storage buffer, then records
`CopyBufferToBuffer` into its readback buffer in the same command list. The
original Deko3D command recorder snapshots the source's CPU bytes before that
compute work executes, so it copies stale depth values.

The repair uses GPU source/destination addresses for GPU-writable buffers.
It orders prior producers, flushes dirty L2 cache lines, performs the GPU copy,
then orders its completion and invalidates shader/L2 caches. The exact frontend
`MapWrite | CopySrc` staging usage retains the existing snapshot path for vertex
and uniform uploads; other usages take the ordered GPU path. These barriers
follow the [Deko3D synchronization implementation](https://github.com/devkitPro/deko3d/blob/master/source/maxwell/gpu_base.cpp).

The regression compiles the complete production copy command case with deferred
GPU execution. The original case reproduces a stale result; the repaired case
passes compute/query/write producer variants, source/destination offsets,
zero-length operations, initialization failures and the retained staging path.
The source audit accepts only complete original, prior audited and new repaired
files for cache migration. This CPU seam does not validate GPU behavior or prove
that stale depth caused the reported character-selection crash. A new private
build and console trial are tracked separately.

The full private rebuild of this repair passes artifact and source/dependency
validation. Candidate `c9f7ff3` replaces `0b4f3c6` on SD at
2026-10-14 22:02:07 UTC (00:02:07 CEST on October 15), with complete size/SHA-256
readback. The previous NRO is backed up and verified against its recorded
metadata before removal; an independent final inventory contains only the new
project candidate. Configuration, extraction marker, game/NAND data and unrelated
applications are preserved. The old NRO and exact debug ELF remain local for
possible crash attribution. No console execution of this candidate is claimed.

```text
depth-copy source: c9f7ff325b9ad8cb1bc52a6af158f5c45a3f0a40
NRO bytes:      74444944
NRO SHA-256:    2e8514ee667b37df34948335f052129b891f788e676e02919d15557d18082d5c
SD path:        sdmc:/switch/WiiCompiled-Switch-native-deko3d-c9f7ff3.nro
```

## Visibility between GPU passes

The next source audit finds missing pass-boundary synchronization. GX EFB copies
sample previously rendered images, and compute passes read earlier GPU writes.
The original recorder starts those passes without ordering preceding work or
invalidating sampled-image/shader caches. Deko3D's automatic post-submit cache
invalidation covers queue flushes, rather than consecutive passes within one
command list. See its [barrier implementation](https://github.com/devkitPro/deko3d/blob/master/source/maxwell/gpu_base.cpp).

The repair inserts a full GPU barrier with image, shader and L2 cache invalidation
before each render/compute pass, after lazy resource and attachment initialization.
This conservative ordering may affect performance; no FPS improvement is claimed.
The complete production pass-begin cases compile into a contract with deferred
producers and persistent stale cache entries. The original cases return stale
data; the repaired cases read the producer's image/storage values and include
attachment initialization before invalidation. Render, compute and initialization
failure cases pass. The earlier complete-file runtime variants remain the only
accepted cache migration inputs.

This contract verifies recorded synchronization and visibility under the CPU
model. It does not reproduce physical Tegra caches or establish the cause of the
character-selection crash. Private build, deployment and console validation of
this change are tracked separately below.

The private native rebuild passes complete artifact/source/dependency validation.
Candidate `4792ed9` replaces `c9f7ff3` on SD at 2026-10-14 22:32:25 UTC
(00:32:25 CEST on October 15). Full new-NRO size/SHA-256 readback precedes the
backup, recorded-metadata verification and removal of the older candidate.
Independent local hashing and a fresh SD directory inventory verify the result.
Configuration and the extraction marker pass identical before/after readbacks;
other applications and game/NAND data are preserved. The previous candidate's
NRO and exact ELF remain locally retained and verified. No new Switch game run
has been observed; character-selection correctness, the initial fault if it
persists, complete races and performance remain unvalidated.

```text
current source: 4792ed92b09fa18d0bbc12a06d64ce0dd63eb8c8
NRO bytes:      74444944
NRO SHA-256:    166e1336d5321f0186dc7898e06a3c5de09149b6e7e21ac75ea03a103beae055
SD path:        sdmc:/switch/WiiCompiled-Switch-native-deko3d-4792ed9.nro
```
