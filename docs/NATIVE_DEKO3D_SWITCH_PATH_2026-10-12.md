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

The DriftDroid tree carries the Switch/Deko3D Dawn patch itself, so the Dawn source side can be
reconstructed from that base commit plus the pinned patch.

UAM is different. The public reference expects a separately prepared
`$HOME/uam/build-switch/libuamcore.a` and exposes the source/build directories to the patched
Dawn backend, but the repository does not publish the complete command sequence or a binary hash
that produced the proven `libuamcore.a`. The public devkitPro UAM repository is currently pinned
at upstream commit `5a5afc2bae8b55409ab36ba45be63fcb73f68993`, but that alone is **not sufficient evidence**
that it is byte-identical to the library used for DriftDroid's measured Switch build.

Therefore the current Deko3D build helper treats Dawn/UAM as explicit external dependencies and
refuses to build when they are absent. A Deko3D NRO is not considered reproducible or eligible
for merge until the local dependency receipt records the exact Dawn base, DriftDroid patch and
UAM source/build hashes. Do not silently substitute a different UAM build.

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

1. Prepare the pinned reference with `scripts/prepare-native-deko3d-reference.sh`.
2. Reconstruct Dawn from `13abc3bc8ea2d3c2050f9e77a12d012108ceee24`, apply the pinned
   DriftDroid Switch/Deko3D patch, and prepare UAM with an attributable source/build receipt.
3. Generate a fresh translation with the pinned DriftDroid translator from the user's own
   `local-product/Assets/main.dol` and `StaticR.rel`; do not reuse old generated C++ blindly.
4. Build Release with Deko3D validation off, threaded GX on and section ordering on.
5. First hardware gate: boot -> Wiimote page -> menu, with exact NRO attribution.
6. Second gate: complete a Time Trial at stock clocks, record frame pacing and CPU/GPU timing.
7. Third gate: 12-kart race at stock clocks; only then compare against the old NVK path.
8. Once correctness is stable, record a representative race profile and test a `MKW_PGO=use`
   product against the non-PGO release.

## What is deliberately not changed yet

The current NVK rendered pipeline, its WiiCompiled pin, bridges, LAN debug server and replay
oracles remain untouched. The new reference is additive until a native Deko3D NRO from this repo
passes build and hardware gates. This avoids trading a known, attributable debugging path for an
untested bulk replacement.
