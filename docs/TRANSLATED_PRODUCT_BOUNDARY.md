# M2 — Translated-product boundary

Status: **hardware-validated and crossed on real Nintendo Switch**. The project links and executes a locally generated WiiCompiled Mario Kart Wii product; the active milestone is translated boot toward PAL `main()`.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Why this boundary exists

WiiCompiled is a static recompiler. A translated Mario Kart Wii product is not a DOL dynamically loaded by the Horizon runtime from the SD card.

At the pinned upstream revision the translator flow is:

1. `translate-recursive` emits translated C++ from locally supplied game inputs;
2. `generate-data-init` emits embedded data-section initialization and `RuntimeConfig.h`;
3. `emit-build-shards` emits the generated native build graph;
4. the generated output is compiled and linked together with the WiiCompiled runtime into one native executable.

For the Switch port, that means the user-owned translated product is linked into the same AArch64 NRO as the Horizon runtime.

## Three separate data domains

The port keeps these concepts separate:

### 1. Build-time user-owned inputs

Local inputs used by WiiCompiled's translator, such as the DOL/REL required by the Mario Kart Wii manifest.

### 2. Build-time translated product

Generated C++ plus generated runtime configuration/data initialization compiled into the NRO.

### 3. Runtime SD data

Host/runtime state under `sdmc:/switch/WiiCompiled-Switch`, including logs, cache/configuration state and fast-track diagnostics.

The SD runtime directory is **not** a loader for translated executable code.

## Public weak seam

`include/translated_product.hpp` defines the translated-product query ABI. The public Nintendo-data-free build supplies a weak definition that reports no linked game product.

This preserves a public CI path that can validate the Horizon runtime without shipping or ingesting Nintendo game-derived content.

## Local strong product path

The private local build replaces the public weak product seam with the generated WiiCompiled product and associated handoff/data initialization.

That path has now been hardware-validated beyond metadata inspection:

- generated data initialization executes;
- translated execution handoff executes;
- PAL `__start` (`0x800060A4`) executes on Switch;
- the local headless platform path reaches `TRANSLATED_EXEC_ENTER` with an active TLS guest context;
- real Wii SDK/native boundaries are reached and fixed iteratively;
- native HLE can call back into translated code, as validated for `IPCCltInit` → `IPCInit`;
- native HLE can publish required guest SDA bookkeeping, as validated for `__OSInitSTM`.

Therefore the previous `WAITING_FOR_TRANSLATED_PRODUCT` / `TRANSLATED_PRODUCT_LINKED` states are historical bootstrap milestones, not the current development stop point.

## Current execution milestone

The active local path is:

```text
local user-owned game inputs
  ↓
WiiCompiled generation
  ↓
AArch64 compile/link into NRO
  ↓
generated data initialization
  ↓
translated execution handoff
  ↓
PAL __start (0x800060A4)
  ↓
Wii SDK / OS bootstrap
  ↓
OSReport → OSGetConsoleType → OSGetResetCode
  ↓
DCZeroRange → IPCCltInit → __OSInitSTM
  ↓
PAL main (0x8000B6B0)  ← not yet proven
```

As of 2026-09-12, real hardware has crossed the translated/native boundaries for `OSReport`, `OSGetConsoleType`, `OSGetResetCode`, `DCZeroRange`, `IPCCltInit`, and `__OSInitSTM`.

The project has **not yet emitted `fast-track-main-reached.txt`**, so `main()` must not be claimed as reached.

## Important boundary lessons from hardware

### Headless host bootstrap

A pre-guest `MAIN_PLATFORM_INIT` Data Abort with an 8 MiB host clear was attributable to the libnx PrintConsole/NV path, not to translated Mario Kart Wii code. The local fast-track now skips that path. Later runs reached translated execution with active guest state, validating that the headless product path is real on hardware.

### Guest-memory contract at native HLE boundaries

`DCZeroRange` exposed a difference between upstream WiiCompiled and the Switch memory slice. Upstream reports an invalid range through `Memory::AccessViolation`; the Switch `Memory::GetPointer` slice returns `nullptr`.

The hardware case `r3 = 0xFFFFFFFF` aligned to `0xFFFFFFE0` and previously caused `memset(nullptr, 0, 0x20)`. The Switch HLE now treats a null guest pointer as the upstream best-effort invalid-range path rather than converting it into a host crash.

### Native-to-translated calls are part of the product boundary

Pinned WiiCompiled's `IPCCltInit` HLE calls translated `IPCInit` (`0x80192F7C`) before advancing the IPC buffer-low pointer by `0x1000`. The Switch port mirrors this behavior instead of treating every native HLE as an isolated leaf.

This is important for later runtime work: a host/native override may still depend on translated guest code and guest-memory side effects.

### Native HLE may publish guest SDA state

Pinned WiiCompiled's `__OSInitSTM` HLE (`0x801AB848`) avoids real Wii `/dev/stm/*` IOS devices but still writes the guest-visible state expected by reset logic: an initialized flag plus two non-zero fake STM handles in the `r13` SDA block. The Switch port mirrors that state and guards the whole range before writing.

This boundary reinforces that a hardware-facing HLE cannot automatically be reduced to a no-op: host I/O may be skipped while guest bookkeeping must still be preserved.

## Diagnostics at this boundary

The local translated path uses durable SD records rather than relying on visible output:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-progress.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-main-reached.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt
sdmc:/switch/WiiCompiled-Switch/fast-track-exception.txt
```

These files make three states distinguishable:

- translated startup is still alive/progressing;
- a specific unsupported translated/native boundary stopped execution;
- a host exception occurred with attributable AArch64/guest context.

`fast-track-main-reached.txt` is the explicit proof marker for PAL `main` at `0x8000B6B0`.

## Hardware history

### 2026-09-10

The public Nintendo-data-free boundary was validated on hardware: Horizon services, guest memory, HostContext, filesystem roots and the weak translated-product seam behaved as expected.

### 2026-09-12

The local generated-product path was exercised on hardware through real translated startup. Hardware blockers observed and subsequently fixed include:

- `0x801A25D0` — `OSReport`;
- `0x8019F33C` — `OSGetConsoleType`;
- `0x801A8A50` — `OSGetResetCode`;
- `0x801A16E4` — `DCZeroRange`;
- `0x80193478` — `IPCCltInit`;
- `0x801AB848` — `__OSInitSTM`.

The day also produced two useful non-dispatch diagnostics:

- a pre-guest PrintConsole/NV-related host crash, removed from the local fast-track by making it headless;
- an active-guest `DCZeroRange` null-pointer crash, fixed by honoring the Switch memory slice's null-return contract.

See `HARDWARE_RESULTS_2026-09-12.md`.

## Local-only content policy

Do not commit or upload:

- DOL/REL game inputs;
- disc images;
- Nintendo keys or firmware;
- extracted copyrighted assets;
- generated translated game C++/data/object output;
- game-containing NRO/ELF artifacts.

Only Nintendo-data-free runtime/platform code, documentation and synthetic probes belong in public CI.

## Validation result

- pinned WiiCompiled submodule: PASS;
- Nintendo-data-free public NRO path: PASS;
- real Switch runtime/GuestFlat/HostContext foundation: PASS;
- local generated data initialization: PASS;
- local translated `__start` execution: PASS;
- headless local platform path reaching translated execution: PASS;
- native HLE → translated dispatch seam: PASS;
- native HLE guest-SDA state publication: PASS;
- first-blocker durable diagnostics: PASS;
- PAL `main()` reached: **NOT YET PROVEN**;
- first rendered frame: **NOT YET PROVEN**.

## Next boundary

The next meaningful boundary is no longer "translated product linked". It is:

1. keep the local fast-track on PAL `__start`;
2. fix each first unsupported boundary using the pinned WiiCompiled semantics;
3. write `fast-track-main-reached.txt` when dispatch reaches `0x8000B6B0`;
4. only then begin systematic post-`main` game-subsystem bring-up.
