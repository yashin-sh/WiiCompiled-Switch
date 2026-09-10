# M2 — Guarded translated data-section initialization

Status: public CI + real-hardware synthetic validation required before the generic handoff checkpoint is complete.

Issue: #21.

Pinned WiiCompiled revision: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Goal

Advance exactly one boundary beyond `TRANSLATED_PRODUCT_LINKED`: allow a separately linked translated product to expose a guarded data-section initializer, invoke only that initializer, verify its result, and stop before static constructors or any broader translated guest execution.

The already hardware-validated metadata ABI remains unchanged at v1. Execution uses a second independent ABI so merely linking/inspecting a translated product cannot accidentally execute it.

## Public Nintendo-data-free probe

Build:

```sh
make -j2 MKW_SYNTHETIC_DATA_INIT=1
```

Output:

```text
WiiCompiled-Switch-synthetic-data-init.nro
```

The synthetic provider exposes strong implementations of both:

```text
mkw_switch_get_translated_product_api
mkw_switch_get_translated_product_handoff_api
```

Its initializer copies a 16-byte synthetic payload into mapped guest MEM1 and verifies the bytes. It contains no Nintendo code, assets or data.

Expected Switch report:

```text
translated product     : LINKED
translated product ABI : expected=1 reported=1
translated product id  : synthetic-data-init-product
translated build       : Nintendo-data-free data-init handoff probe
data-init handoff      : ENABLED
data-init provider     : LINKED
data-init handoff ABI  : expected=1 reported=1
data initializer       : AVAILABLE
data sections init     : PASS
stop point             : DATA_SECTIONS_INITIALIZED
```

Audio and graphics remain `STUBBED`. Reaching `DATA_SECTIONS_INITIALIZED` does **not** mean Mario Kart Wii code, constructors or the game entry point executed.

## Why generated WiiCompiled data init can use the Switch memory slice

At the pinned revision, `generate-data-init` emits `data_sections_init.cpp` calling:

```text
Memory::Contains(...)
Memory::GetPointer(...)
memcpy(...)
```

The Horizon memory slice exposes those same operations over the hardware-validated heap-backed MEM1/MEM2 storage. `include/memory.h` is a narrow compatibility shim that deliberately binds this generated source to the Switch memory implementation instead of the larger desktop declaration.

The translator also emits `data_sections_init_blobs.S` plus binary blobs. Blob references are absolute paths, so they can be assembled directly from the local generated directory.

## Local-only user-owned data checkpoint

Never put the following in Git, issues, PRs or public CI artifacts:

- `main.dol`;
- `StaticR.rel`;
- disc images;
- generated data blobs/C++;
- the resulting local NRO.

On a Linux development machine, place your clean PAL RMCP01 files at:

```text
local-product/Assets/main.dol
local-product/Assets/StaticR.rel
```

Then run:

```sh
bash scripts/prepare-local-data-init.sh
make -j2 MKW_LOCAL_PRODUCT=1
```

The preparation script:

1. verifies the pinned WiiCompiled submodule revision;
2. builds the pinned .NET translator;
3. validates the local DOL/REL against the upstream RMCP01 SHA-256 pins;
4. runs `generate-data-init` using the tracked local manifest template;
5. verifies `data_sections_init.cpp`, `data_sections_init_blobs.S` and `RuntimeConfig.h` were produced;
6. rejects non-ELF blob assembly output.

The build then compiles only the generated data initializer/blob assembly plus the generic local handoff adapter into:

```text
WiiCompiled-Switch-local-product.nro
```

That NRO contains user-owned game-derived data and must remain local.

Launch it through hbmenu application/title-override mode with full memory. The intended second hardware checkpoint is:

```text
translated product     : LINKED
data-init handoff      : ENABLED
data-init provider     : LINKED
data initializer       : AVAILABLE
data sections init     : PASS
stop point             : DATA_SECTIONS_INITIALIZED
```

Only `runtime-bootstrap.txt` should be shared back for validation.

## Linux requirement for this checkpoint

WiiCompiled's blob generator emits platform-specific assembly section syntax based on the OS running the translator. The Switch toolchain consumes ELF/AArch64 objects, so this checkpoint intentionally requires Linux, where the generated blob assembly uses `.rodata` ELF syntax.

## Deliberately deferred

This checkpoint does not yet:

- run `translate-recursive`;
- link the generated translated function shards;
- initialize the persistent PPC CPU context for game code;
- run DOL or REL static constructors;
- call the translated game entry point;
- enable graphics or audio.

Those are separate, attributable checkpoints after real-hardware data initialization is proven.
