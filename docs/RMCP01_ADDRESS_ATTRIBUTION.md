# RMCP01 address attribution helper

This repository includes a local-only helper for turning a hardware-observed
PAL RMCP01 guest address into useful public reverse-engineering context without
changing the blocker-driven implementation policy.

The helper uses public `doldecomp/mkw` metadata and optionally emits
`decomp-toolkit` (DTK) follow-up commands for local game files.

It never requires Nintendo data for its default attribution path.

## Why this exists

A real-Switch blocker often starts as only an address:

```text
target = 0x80170A4C
```

The useful questions are then:

- is the address in `main.dol` or `StaticR.rel`?
- which translation unit owns the range?
- does `doldecomp/mkw` publish a PAL symbol/source range for it?
- which DTK command should be run next if binary-level inspection is needed?

The helper answers those questions before any HLE/runtime implementation is
attempted.

Hardware evidence and the pinned WiiCompiled revision remain authoritative for
deciding what behavior may be implemented.

## First use

From the WiiCompiled-Switch repository:

```bash
python3 scripts/attribute-rmcp01-address.py --fetch 0x80170A4C
```

`--fetch` is explicit. It clones the public `doldecomp/mkw` repository into:

```text
.deps/analysis/mkw
```

Later calls do not need network access:

```bash
python3 scripts/attribute-rmcp01-address.py 0x8055531C
```

The helper also accepts an existing checkout:

```bash
python3 scripts/attribute-rmcp01-address.py \
  --doldecomp ~/dev_perso/mkw \
  0x8055531C
```

or:

```bash
export MKW_DOLDECOMP_DIR=~/dev_perso/mkw
python3 scripts/attribute-rmcp01-address.py 0x8055531C
```

## Output

Typical output contains:

```text
RMCP01 address attribution
address             : 0x8055531C
module              : StaticR.rel
section             : .text
translation unit    : util/ModuleSymbols.cpp
translation range   : 0x8055531C..0x80555464
TU offset           : +0x0
symbol              : ...
symbol range        : ...
symbol offset       : ...
source              : ...
confidence          : ...
doldecomp revision  : ...
dtk                 : ...
```

The symbol/source fields are best-effort. The split-range attribution is still
useful when a function has not yet been decompiled or annotated by
`doldecomp/mkw`.

## JSON mode

For scripts or future blocker-report automation:

```bash
python3 scripts/attribute-rmcp01-address.py \
  --json \
  0x8055531C
```

The JSON output includes the selected split, symbol/source match, doldecomp
revision, confidence class, and DTK follow-up commands.

## DTK integration

If `dtk` is installed or `MKW_DTK_BIN` points to it, the helper reports
the detected binary and prints the appropriate next binary-inspection command.

Example:

```bash
export MKW_DTK_BIN=~/bin/dtk
python3 scripts/attribute-rmcp01-address.py 0x8055531C
```

If a user-owned RMCP01 disc image is available locally, it can be supplied
only as a local path:

```bash
python3 scripts/attribute-rmcp01-address.py \
  --disc ~/games/RMCP01.rvz \
  0x8055531C
```

The helper does not upload, copy, hash, or commit the disc image. It only uses
the path to construct a DTK VFS command such as `dtk rel info` or
`dtk dol info`.

## Network behavior

The helper performs no network access by default.

Network access occurs only with:

```text
--fetch
```

and is limited to cloning or fast-forward updating the public
`https://github.com/doldecomp/mkw.git` checkout.

An existing checkout with local/divergent commits is never hard-reset; the
fast-forward update fails instead.

## Nintendo-data boundary

Never commit or upload:

- `main.dol`;
- `StaticR.rel`;
- ISO/RVZ/WBFS images;
- extracted game data;
- DTK-generated artifacts derived from the user's game.

The repository's existing `.gitignore` already excludes the common local
binary extensions and `.deps/`.

## Decision rule

Use the helper in this order for every ambiguous blocker:

1. capture the exact durable blocker from real Switch hardware;
2. run the address attribution helper;
3. inspect the exact pinned WiiCompiled semantics;
4. use DTK locally only when binary/module details remain ambiguous;
5. optionally use decomp.me manually for a difficult PPC function;
6. implement only the exact hardware-proven boundary;
7. require the normal five-workflow CI gate before merge.

The helper improves attribution speed. It does not authorize pre-porting
neighboring functions.

## Self-test

The parser includes a Nintendo-data-free self-test:

```bash
python3 scripts/attribute-rmcp01-address.py --self-test
```

It validates both a synthetic `main.dol` range and a synthetic
`StaticR.rel` range.
