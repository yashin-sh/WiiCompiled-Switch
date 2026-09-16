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