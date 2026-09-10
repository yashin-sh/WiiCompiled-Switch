# M2 translated function-shard link-only checkpoint

This checkpoint proves that WiiCompiled's locally generated Mario Kart Wii base
function shards can be compiled by devkitA64 and linked into a libnx NRO without
executing translated guest code.

## Local-only generation

User-owned PAL RMCP01 inputs and every generated output remain under the ignored
`local-product/` tree. `scripts/prepare-local-function-shards.sh` passes explicit
paths to the pinned translator for:

- `--outdir local-product/generated/functions`;
- `--output-metadata local-product/generated/base_translation_output.json`;
- `--base-metadata` and `--base-functions-dir`;
- `--native-source-dir third_party/WiiCompiled/runtime/src`;
- `--out local-product/generated/build_shards`.

This is required because WiiCompiled's CLI defaults its build-shard metadata and
function inputs to `<workspace_root>/generated/...`, while this port deliberately
keeps game-derived output below `local-product/generated/`.

## Link-only policy

Build with:

```sh
make -j2 MKW_LOCAL_FUNCTION_SHARDS=1
```

The mode consumes only translated function aggregates:

- `build_shards/base_common/*.cpp`;
- `build_shards/base_portable_sensitive/*.cpp` when present.

It deliberately excludes `base_registration` and `base_dispatch`. Those files
contain global registrar objects whose constructors publish translated function
records before `main`; importing them would cross the execution-lifecycle
boundary before it has been staged on Horizon.

The linker uses section GC and explicitly retains PAL RMCP01 `__get_debug_bba`
(guest entry `0x8000609C`) as proof that real translated code survives into the
ELF/NRO. The runtime does not call that function in this checkpoint and still
stops after the already validated generated data initializer.

## Horizon compatibility seams

The pinned WiiCompiled translated ISA is Clang-oriented while devkitA64 uses GCC.
A build-scoped compatibility preinclude maps the single `ext_vector_type` use to
GCC's `vector_size` equivalent.

Translated code also must not pull the desktop `Memory`/flat-map implementation.
The Switch `ppc_isa_memory.h` keeps translated helpers on the checked,
heap-backed `Memory::*` path. The link-only `abi_bridge.h` provides only the
compile-time translated traits/direct-call surface; state-free runtime dispatch
is disabled and generic dynamic dispatch aborts if accidentally invoked.

## Public CI

Public CI cannot contain or generate Mario Kart Wii translation output. Instead,
a Nintendo-data-free synthetic translated leaf exercises the same devkitA64 GCC,
`ppc_runtime.h`, link-only ABI seam, checked memory seam and vector compatibility.
CI force-retains the synthetic symbol and verifies it is strong text with
`aarch64-none-elf-nm`.

## Expected local proof

After generation and build, verify locally:

```sh
$DEVKITPRO/devkitA64/bin/aarch64-none-elf-nm \
  WiiCompiled-Switch-local-function-link.elf | grep -E ' T __get_debug_bba$'
```

A successful NRO build plus that symbol proof completes the remaining issue #21
compile/link acceptance item. A real-Switch run should remain at
`DATA_SECTIONS_INITIALIZED`; first translated execution is a later guarded
checkpoint.

Never upload the local generated tree, DOL/REL inputs, ISO/WBFS/RVZ, or the
resulting game-containing NRO/ELF.
