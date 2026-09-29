# RMCP01 GX texture callsite scan

This workflow inventories direct PAL RMCP01 callsites for the GX texture-object
functions that currently dominate the rendered fast-track frontier, without
committing Nintendo code or game data.

The scanner is static evidence only. Real-Switch logs remain authoritative for
runtime descriptor values and for deciding whether a hardware gate is crossed.

## Public references

Two public sources are useful together:

1. `doldecomp/mkw` is the readable PAL RMCP01 decompilation and provides
   symbol/split/source attribution for `main.dol` and `StaticR.rel`.
2. MKW-SP publishes a PAL Ghidra metadata export (`pal.raw.xml`). Its README
   explicitly requires importing a user-owned PAL MEM1 dump before adding the
   XML. The public XML therefore supplies function/symbol/type metadata, not the
   game bytes themselves.

The currently public decompilation exposes explicit `GXInitTexObjLOD` calls
in at least:

- `TPLGetGXTexObjFromPalette` in `lib/rvl/tpl/tpl.c`;
- `nw4r::ut::CharWriter::LoadTexture` in
  `lib/nw4r/ut/ut_charWriter.cpp`.

The MKW-SP PAL Ghidra metadata names `TPLGetGXTexObjFromPalette` at
`0x801B7544` and `CharWriter_PrintGlyph` at `0x800B3C70`.

Those public sources are valuable for naming and context, but they do not
publish the complete set of runtime `GXTexObj` instances or the exact
`0x90......` object/descriptors seen by the Switch fast-track. Those values
remain user-owned runtime evidence.

## Scanner

The repository provides:

```bash
python3 scripts/scan-rmcp01-gx-texture-calls.py --self-test
```

By default it scans direct PPC branch-and-link calls to:

```text
0x801707F8  GXInitTexObj
0x80170A4C  GXInitTexObjLOD
0x80170B50  GXInitTexObjWrapMode
0x80170F2C  GXLoadTexObj
```

It accepts either:

- a raw MEM1 image with `--mem1`; or
- a DTK-merged ELF with `--elf`.

The scanner decodes PPC opcode 18 direct branch-and-link instructions, resolves
their exact guest targets, then optionally attributes the callsite through
public doldecomp and Ghidra metadata.

It does **not** assume that a static callsite was executed and it does not
predict a runtime descriptor that was not observed.

## Recommended local workflow

Use the existing gitignored user-owned assets:

```text
local-product/Assets/main.dol
local-product/Assets/StaticR.rel
```

Merge the DOL and REL locally with DTK:

```bash
dtk rel merge \
  local-product/Assets/main.dol \
  local-product/Assets/StaticR.rel \
  -o /tmp/rmcp01-merged.elf
```

Prepare/update the public doldecomp checkout if needed:

```bash
python3 scripts/attribute-rmcp01-address.py --fetch 0x80170A4C
```

Then enumerate the direct GX texture callsites:

```bash
python3 scripts/scan-rmcp01-gx-texture-calls.py \
  --elf /tmp/rmcp01-merged.elf \
  --doldecomp .deps/analysis/mkw
```

For machine-readable output:

```bash
python3 scripts/scan-rmcp01-gx-texture-calls.py \
  --elf /tmp/rmcp01-merged.elf \
  --doldecomp .deps/analysis/mkw \
  --json > /tmp/rmcp01-gx-texture-callsites.json
```

If the public MKW-SP `pal.raw.xml` is available locally, add:

```text
--ghidra-xml /path/to/pal.raw.xml
```

This can name callers that are not yet represented by a readable doldecomp
source symbol.

## MEM1 mode

The same scanner can inspect a user-owned PAL MEM1 image:

```bash
python3 scripts/scan-rmcp01-gx-texture-calls.py \
  --mem1 /path/to/pal-mem1.bin \
  --mem1-base 0x80000000 \
  --doldecomp .deps/analysis/mkw \
  --ghidra-xml /path/to/pal.raw.xml
```

This mode is particularly useful when the REL has already been relocated in
memory.

## What this gives us

The result answers a different question from the hardware blocker log:

- scanner: *which exact code locations can directly call the GX texture APIs?*
- hardware: *which runtime object/descriptor tuple actually arrived next?*

Once the full callsite list is available, future hardware logs can be grouped
by caller/function family instead of treating every LOD as an unrelated event.
That should reveal whether the observed sequence is a bounded texture
initialization loop and whether several future LOD/wrap gates come from the
same public routine.

## Limitations

- Only direct PPC `bl` calls are enumerated. Function pointers, branch
  trampolines and other indirect dispatch are intentionally not guessed.
- Public MKW-SP Ghidra XML is metadata, not Nintendo code; call instruction
  bytes still come from the user's own RMCP01.
- Static callsites do not authorize runtime HLE widening.
- Exact `GXTexObj` words and object addresses remain hardware/runtime
  evidence unless independently derived from the user's local binary state.
