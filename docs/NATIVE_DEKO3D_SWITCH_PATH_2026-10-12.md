# Native Deko3D Switch path — 2026-10-12

## Decision

For the playable/performance track, prefer the proven static-recompilation path used by
DriftDroid on Switch:

1. translate the user-owned PAL RMCP01 PowerPC program ahead of time to native AArch64;
2. keep Aurora GX as the Wii/GameCube graphics compatibility layer;
3. use Dawn's Switch-specific Deko3D backend and link **release** `deko3d` by default;
4. enable threaded GX and overlap frame encoding with the next guest frame;
5. retain native AArch64 paired-single/PSQ helpers, hot-section ordering and optional PGO;
6. keep native THP decode, buffered audio, shader caching/prewarm, bulk DVD I/O/read-ahead and
   Switch-specific thread placement from the proven reference implementation.

The existing Aurora -> Dawn/WebGPU -> Vulkan/NVK route remains available as a diagnostic
fallback until the Deko3D product passes the project's private rendered-build gate and real
Switch hardware validation. Do not delete the existing path during migration.

This is still **static recompilation**, not Dolphin, an interpreter or a runtime JIT.

## Reproducible reference

`third_party/DriftDroid` is pinned to:

```
8b33d31016a8b8735fe35b4fb633c491d33774c6
```

That pin includes the public Switch implementation that demonstrated full races. It is kept as a
separate submodule instead of replacing `third_party/WiiCompiled`, so the current exact-source
CI, replay and hardware evidence remain reproducible while the optimized path is brought up.

### Graphics dependency provenance

The pinned Aurora source identifies Dawn release `v20260603.191052`. The corresponding public
`encounter/dawn-build` release states that it was built from upstream `google/dawn` commit:

```
13abc3bc8ea2d3c2050f9e77a12d012108ceee24
```

The DriftDroid tree carries the Switch/Deko3D Dawn patch and the matching Abseil/newlib Switch
patch. `scripts/prepare-native-deko3d-deps.sh` reconstructs this source locally, initializes the
pinned Dawn dependencies, applies only those authorized patches and builds the static
`libwebgpu_dawn.a` with only the native Deko3D backend enabled.

The strict C++20 Switch build also needs audited portability fixes for Dawn's own pinned
Abseil, Tint identifier classification and Dawn environment/sleep utilities. Switch sleep uses
`svcSleepThread` with microseconds widened to 64 bits before conversion to nanoseconds. The
helper verifies these source changes and records its patcher hash in the dependency receipt.
The Deko3D configuration explicitly enables Tint's desktop GLSL writer even though the GL/GLES
backends are disabled. The pinned UAM memory API uses a size query and a 256-byte-aligned output
buffer; its adapter is checked against the complete shader-compiler file from the pinned Dawn
patch, in addition to checking the remaining patch hunks.
Message normalization retains `strnlen` semantics on Switch with an ISO C++ bounded scan, so
explicit-length messages never read beyond their provided length.

The missing UAM provenance has also been resolved without inventing a private build recipe.
`averne/libuam` is a public fork of devkitPro UAM that turns the compiler into a static library
for Nintendo Switch. We pin it to:

```
f8c9eef01ffe06334d530393d636d69e2b52744b
```

Its public Switch packaging recipe builds that source with Meson/devkitA64 into `libuam.a`.
Our dependency helper reproduces that cross-build locally instead of relying on DriftDroid's
previously assumed `$HOME/uam/build-switch/libuamcore.a`.

The dependency helper writes `.deps/native-deko3d/native-deko3d-toolchain.receipt` containing the
Dawn/UAM commits, patch hashes, worktree-diff hashes, archive hashes and local toolchain versions.
The final NRO receipt includes the hash of this dependency receipt, so hardware evidence can be
traced back to the exact graphics toolchain.

## One-command local candidate

With devkitPro/devkitA64, libnx, release Deko3D and the Switch portlibs required by the pinned
Aurora provider installed, plus .NET 8, CMake/Ninja, Meson, Python/Mako, Bison and Flex on the
host, and the user's own PAL inputs already under
`local-product/Assets/main.dol` and `local-product/Assets/StaticR.rel`, the normal path is:

```bash
bash scripts/build-local-native-deko3d.sh
```

That command, by default:

1. prepares/builds pinned Dawn + `averne/libuam` under `.deps/native-deko3d/`;
2. creates a clean DriftDroid workspace and freshly translates the user's own game program;
3. builds the optimized Release Switch product;
4. packages the private NRO under `local-product/native-deko3d/`;
5. writes SHA-256 and dependency provenance beside it.

Advanced reuse is opt-in with `MKW_SKIP_DEKO3D_DEPS=1` and/or `MKW_SKIP_TRANSLATE=1`; the build
still refuses unattributed or incorrectly pinned dependencies.

ANGLE and SwiftShader remain uninitialized because their backends are disabled; that explicit
exception does not permit other missing, moved or dirty dependencies. A custom
`MKW_DEKO3D_DEPS_DIR` is shared by dependency preparation and translation preparation.
Reuse checks compare the current dependency sources and archives with their build receipt and
the translated inputs with their recorded reference/input hashes. Workspace cleanup refuses
paths that would remove the checkout, inputs or other protected build directories.

## Why this path

The Switch is CPU-bound in full races. The reference implementation records several hardware
wins that directly target that limit:

- release Deko3D instead of its validating debug library removed a large per-call CPU cost;
- threaded GX allows the guest/game thread and GX decode/submission work to overlap;
- finished-frame encoding can overlap the next guest frame;
- native AArch64 paired-single/PSQ helpers avoid expensive generic translations in hot code;
- section ordering places the measured race working set together, reducing A57 iTLB/L2 pressure;
- PGO is supported as a second-stage optimization after a representative hardware profile;
- native THP decoding and multi-buffer audio remove avoidable CPU stalls;
- bulk DVD DMA/read-ahead reduces filesystem/copy overhead;
- shader cache/prewarm moves compilation work away from gameplay where possible.

The reference's public Switch beta reports Time Trial at 50–60 FPS on stock clocks and full
12-kart races around 30 FPS stock, with materially higher performance under overclock. Those are
reference measurements, **not yet claims for WiiCompiled-Switch**.

## Migration rules

The optimized path must not weaken this repository's validation discipline:

- game data, generated translations, NROs and raw hardware captures stay private;
- release Deko3D is the performance default, but `AURORA_SWITCH_DEKO3D_DEBUG=ON` remains available
  for diagnosing GPU validation failures;
- capture/discovery/debug instrumentation stays opt-in and must not be used for release FPS
  measurements;
- every backend change still needs the private rendered build plus all published exact-HEAD
  checks before merge;
- real hardware evidence must attribute the exact NRO by size and SHA-256;
- no FPS or playability claim is accepted until measured on this repository's exact candidate.

## Bring-up sequence

1. Run `bash scripts/build-local-native-deko3d.sh` to produce the first exact candidate.
2. First hardware gate: boot -> Wiimote page -> menu, with exact NRO attribution.
3. Second gate: complete a Time Trial at stock clocks, record frame pacing and CPU/GPU timing.
4. Third gate: 12-kart race at stock clocks; only then compare against the old NVK path.
5. Once correctness is stable, record a representative race profile and test a `MKW_PGO=use`
   product against the non-PGO release.
6. Only if profiling shows material Dawn frontend CPU cost should a direct Aurora -> Deko3D
   rewrite be considered.

## What is deliberately not changed yet

The current NVK rendered pipeline, its WiiCompiled pin, bridges, LAN debug server and replay
oracles remain untouched. The new path is additive until a native Deko3D NRO from this repo
passes build and hardware gates. This avoids trading a known, attributable debugging path for an
untested bulk replacement.
