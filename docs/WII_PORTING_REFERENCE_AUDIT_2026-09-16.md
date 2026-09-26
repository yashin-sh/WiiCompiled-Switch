# Wii porting reference audit — 2026-09-16

This document records external GameCube/Wii reverse-engineering and static-recompilation projects that are useful as **secondary engineering references** for WiiCompiled-Switch.

The primary runtime contract for blocker-driven work remains the pinned WiiCompiled revision:

`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`

Real Switch hardware evidence remains authoritative for deciding **when** a boundary is implemented. External projects are references for architecture, naming, formats, and validation strategies; they are not a reason to pre-port adjacent subsystems.

## Reference hierarchy

### 1. WiiCompiled — primary runtime semantics

Use pinned WiiCompiled first for every native/HLE blocker reached by the local fast-track. Its exact PAL address mapping and HLE behavior define the current implementation target unless hardware proves a Switch-specific mismatch.

Do not replace the current static-recompilation path with another recompiler. The project has already reached PAL `main()` and is advancing post-main with a functioning AArch64 runtime, guest memory model, HostContext-backed guest thread continuation, scheduler bridges, NAND/DVD bootstrap, VI/GX bootstrap, and input HLE.

### 2. doldecomp/mkw — primary RMCP01 game-structure reference

Repository: `doldecomp/mkw`

Useful properties:

- targets Mario Kart Wii specifically;
- supported game version is `RMCP01` (PAL), matching WiiCompiled-Switch;
- tracks both `main.dol` and `StaticR.rel`;
- provides symbol/split/decompilation context that can help identify guest functions, structures, and REL ownership without guessing from raw addresses.

Use it to improve blocker attribution and to understand MKW-specific structures or module boundaries. Do not copy game-derived assets or generated proprietary output into this repository.

### 3. decomp-toolkit (dtk) — DOL/REL/disc analysis tool

Repository: `encounter/decomp-toolkit`

Useful capabilities include:

- DOL section and function-boundary analysis;
- signature identification for common SDK/Metrowerks functions;
- relocation and section analysis;
- direct disc image access/extraction;
- REL inspection and DOL+REL merge workflows;
- archive/VFS tooling.

Potential WiiCompiled-Switch uses:

- validate PAL address/module ownership when raw blockers become ambiguous;
- inspect `StaticR.rel` and future REL-backed execution paths;
- validate local-only disc/FST/resource layouts from the user's own dump;
- produce analysis artifacts locally without committing Nintendo data.

DTK is an analysis/build-support tool, not a replacement runtime.


#### Local attribution helper

The repository now provides:

```bash
python3 scripts/attribute-rmcp01-address.py --fetch 0x80170A4C
```

The helper consumes public `doldecomp/mkw` split/source metadata to classify
an observed RMCP01 address as `main.dol` vs `StaticR.rel`, identify the
owning translation-unit range, and recover a PAL symbol/source range when one
is publicly annotated. If DTK is installed, it also emits the appropriate
local `dol info` / `rel info` follow-up command.

This is an attribution accelerator only. It does not change the rule that
hardware decides which boundary is implemented and pinned WiiCompiled defines
the runtime behavior to mirror.

Full usage: `docs/RMCP01_ADDRESS_ATTRIBUTION.md`.

### 4. NWiiRecomp — secondary architecture reference only

Repository: `BlackLineInteractive/NWiiRecomp`

Its documented architecture is useful because it independently tackles many GameCube/Wii runtime problems relevant to later WiiCompiled-Switch milestones:

- DOL parsing and function-boundary analysis;
- PPC-to-C++ recompilation;
- mid-function entry dispatch using guest PC state;
- interpreter fallback for code created/copied at runtime;
- VI/DI/SI/EXI/DSP/AI/MI and Wii IPC modeling;
- IOS HLE for `/dev/di`, `/dev/fs`, `/dev/es`, `/dev/stm`, and `/dev/usb`;
- virtual-disc access;
- Wii/GC low-memory setup including arena/FST/BI2 state;
- controller/SI behavior;
- GX WGPIPE parsing and graphics state tracking;
- headless boot testing.

These concepts are especially relevant to three future areas:

1. real local DVD/FST/resource publication;
2. `StaticR.rel` / dynamic-code and module handling;
3. replacing the current `GX_HLE_FIFO_Write*` sink with a real graphics path.

#### Licensing boundary

NWiiRecomp uses a custom license with non-commercial restrictions, redistribution conditions, and additional restrictions. Treat it as an **architecture/reference source**, not as code to copy, vendor, link, or adapt directly into this GPL-3.0 repository unless compatibility is explicitly resolved first.

For WiiCompiled-Switch work:

- do not copy NWiiRecomp source code into the repository;
- do not vendor or link the runtime;
- document concepts independently;
- implement behavior from Wii hardware specifications, pinned WiiCompiled semantics, MKW evidence, and our own Switch runtime architecture.

## Concrete follow-up tracks

### DVD/FST/resources — #154

Issue #154 defines the local-only architecture and acceptance criteria for publishing a real RMCP01 DVD FST/data mapping when hardware resource loading requires it, without fabricating FST data or committing game content.

Useful references: pinned WiiCompiled, DTK disc/VFS tooling, NWiiRecomp virtual-disc/FST architecture, Wii disc-format documentation.

### StaticR.rel / REL modules — #155

Issue #155 prepares symbol/module ownership, relocation requirements, DOL-vs-REL attribution, and local build/link strategy before hardware first reaches an unsupported REL-backed boundary.

Useful references: `doldecomp/mkw`, DTK REL analysis/merge commands, pinned WiiCompiled dispatch/runtime behavior.

### GX backend

Do not create another broad GX issue. Existing issues already cover:

- #4 — Switch graphics backend strategy;
- #109 — release-safe FIFO bounds;
- #110 — `GXVtxFmt` merge correctness;
- #111 — line-strip vertex-count safety;
- #112 — indexed XF visibility/correctness.

NWiiRecomp's WGPIPE/state/shader architecture is useful as a conceptual comparison for #4, but the Switch backend remains a WiiCompiled/Aurora design decision and should not import NWiiRecomp code.

## Decision rule

For every future blocker:

1. use real Switch hardware evidence to identify the exact boundary;
2. inspect exact pinned WiiCompiled semantics;
3. use `doldecomp/mkw` and DTK to improve MKW/module/symbol attribution where useful;
4. consult NWiiRecomp only as an independent architecture reference for complex hardware/runtime subsystems;
5. implement the narrowest behavior required in WiiCompiled-Switch;
6. keep public CI Nintendo-data-free;
7. merge only after the existing five-workflow CI contract passes.

This preserves the current fast-track discipline while giving later filesystem, REL, input, and graphics work better reference material.

## 2026-09-17 addendum — Aurora and `new-coke/strikers`

A targeted audit of current `encounter/aurora` and `new-coke/strikers` is recorded in `STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md`.

### Aurora — preferred reusable GX reference

Repository: `encounter/aurora`

Aurora is MIT-licensed and is the compatibility layer already targeted by pinned WiiCompiled's desktop GX runtime. At the exact WiiCompiled pin, `GX_HLE_FIFO_Write8/16/32/Float` feed `HleFifoWrite`, which already parses GX FIFO/state/draw traffic and submits through Aurora.

This materially changes the M3 implementation rule: **do not build a second GX FIFO parser for Switch unless the pinned decoder is proven unusable**. The current Switch sink is a temporary fast-track substitution, not the intended first-frame architecture.

Aurora upstream is still WebGPU/Dawn- and SDL3-oriented and has no audited native Deko3D/NXVK backend in this review. That keeps #4's backend question open, but narrows the high-level renderer question substantially.

### `new-coke/strikers` — concrete Aurora/DVD case study

Repository: `new-coke/strikers`

Strikers is a native source port from decompiled Super Mario Strikers code, not a static recompilation. It is therefore **not** a PPC/runtime reference for WiiCompiled-Switch.

It is useful as a concrete case study because it demonstrates:

- a complete Nintendo SDK game running through Aurora GX;
- a small explicit compatibility surface for GX calls Aurora does not provide;
- startup-aware shader/pipeline compilation handling;
- one logical DVD/FST index fed from either an extracted data tree or a disc image;
- separation between game-facing DVD APIs and host storage/read implementation.

For #154, use that storage/index separation as a design pattern while preserving pinned WiiCompiled's exact guest-visible MEM2/FST/low-memory contract.

For #4, use Strikers as evidence that Aurora GX is a viable high-level compatibility layer. The first Horizon graphics probe is now tracked narrowly in #162: preserve pinned WiiCompiled `HleFifoWrite`/Aurora GX and test Dawn/WebGPU → Vulkan/Mesa/NVK on real Switch before paying the cost of a direct Deko3D Aurora backend.

### Strikers licensing boundary

The Strikers README states that the author's original porting code/tools/docs are offered under CC0 only to the extent the author owns them, while reconstructed game code can remain subject to third-party rights.

Therefore:

- do not copy reconstructed Strikers game code or assets;
- prefer upstream MIT-licensed Aurora for reusable implementation code;
- use Strikers for architecture, compatibility-gap discovery and validation strategy;
- independently implement RMCP01 behavior from pinned WiiCompiled, real hardware, public formats and our own code.

### Updated practical reference order

For the current project phase:

1. pinned WiiCompiled — exact runtime/HLE/GX-decoder semantics;
2. real Switch hardware — implementation gate;
3. `doldecomp/mkw` — RMCP01 structure/module attribution;
4. DTK — analysis and disc/REL tooling;
5. upstream Aurora — reusable GX compatibility layer;
6. Strikers — concrete Aurora GX + DVD/FST integration case study;
7. NWiiRecomp — independent architecture reference under its custom-license restrictions.
