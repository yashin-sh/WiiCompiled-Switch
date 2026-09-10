# M2 — Translated-product boundary

Status: implementation slice for issue #18. CI and real-hardware validation are required before this boundary is considered complete.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Why this boundary exists

WiiCompiled is a static recompiler. A translated Mario Kart Wii product is not a DOL that the Horizon runtime discovers and loads from the SD card.

At the pinned upstream revision the translator flow is:

1. `translate-recursive` emits translated C++ from the locally supplied DOL;
2. `generate-data-init` emits the embedded data-section initializer and `RuntimeConfig.h`;
3. `emit-build-shards` emits the generated native build graph;
4. the generated output is compiled and linked together with the WiiCompiled runtime into one native executable.

The upstream MKWii manifest expects user-owned source inputs such as `Assets/main.dol` and `Assets/StaticR.rel`, and writes generated output under `generated/`.

Those source inputs and game-derived generated outputs are local build material. They are never part of this repository or its public CI artifacts.

## Three separate data domains

The Switch port keeps these concepts separate:

### 1. Build-time user-owned inputs

Inputs used only by the WiiCompiled translator on the user's machine, for example the DOL/REL required by the pinned MKWii manifest.

### 2. Build-time translated product

Generated C++ plus generated runtime configuration/data initialization compiled into the same NRO as the runtime.

### 3. Runtime SD data

Host/runtime state that may legitimately live under:

`sdmc:/switch/WiiCompiled-Switch`

The current Horizon boundary creates explicit roots:

- `Logs/`
- `Cache/`
- `Config/`
- `NAND/`

None of these directories is a translated-code loader.

## Link seam

`include/translated_product.hpp` defines ABI version 1 and the C-linkage query:

`mkw_switch_get_translated_product_api()`

The repository build supplies a weak default definition that returns `nullptr`. That is the Nintendo-data-free CI stub.

A future local translated-product adapter can provide a strong definition of the same symbol and return metadata describing the product linked into that NRO. The linker then replaces the weak public stub without changing common Horizon runtime code.

The current API is intentionally metadata-only:

- ABI version;
- product identifier;
- build description.

Inspection has no side effects. It does **not** initialize generated data sections and does **not** enter translated code. Those actions belong to the next execution-handoff slice after this boundary is hardware validated.

## Bootstrap states

If the Horizon core is ready and no product overrides the weak stub:

`WAITING_FOR_TRANSLATED_PRODUCT`

If an ABI-compatible local product adapter is linked:

`TRANSLATED_PRODUCT_LINKED`

That second state still means only that the product seam is present. It is not yet a claim that Mario Kart Wii code was executed.

If a product is linked with the wrong ABI version, the bootstrap deliberately stops as a failure instead of guessing compatibility.

## Runtime report

`runtime-bootstrap.txt` now records:

- translated-product state;
- expected and reported product ABI;
- product id/build description;
- runtime data root;
- Logs/Cache/Config/NAND roots;
- final translated-product stop point.

For the public Nintendo-data-free build, the expected product lines are:

```text
translated product     : NOT LINKED
translated product ABI : expected=1 reported=0
translated product id  : <none>
translated build       : Nintendo-data-free stub
stop point             : WAITING_FOR_TRANSLATED_PRODUCT
```

## Local-only content policy

The root `.gitignore` explicitly excludes local generated/product directories and DOL/REL inputs in addition to disc-image/key formats already excluded.

Do not commit or upload:

- DOL/REL game inputs;
- disc images;
- Nintendo keys or firmware;
- extracted copyrighted assets;
- generated translated game C++/data output;
- a game-containing NRO artifact.

Only Nintendo-data-free runtime/platform code and synthetic probes belong in public CI.

## Validation for this slice

CI must:

- build the NRO with the weak Nintendo-data-free stub;
- verify the pinned WiiCompiled submodule;
- contain no local translated product or game-derived input.

Real Switch validation must then show the existing M2 core still READY/PASS and the new stop point:

`WAITING_FOR_TRANSLATED_PRODUCT`

Only after that hardware report is returned should issue #18 be considered complete.

## Next execution slice

After this boundary passes on hardware, the next local-only path is:

1. generate the MKWii translated product with the pinned WiiCompiled translator from a user-owned dump;
2. provide a strong translated-product adapter in the local build;
3. cross-compile/link the generated product into the AArch64/libnx NRO;
4. initialize the generated data sections;
5. stop at the earliest safe point immediately before or after the first translated entry-point handoff;
6. use the first real runtime failure to drive the next missing HLE/thread/filesystem dependency.

Graphics/Aurora and Audren remain outside this slice.
