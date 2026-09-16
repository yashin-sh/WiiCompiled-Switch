# Hardware result — PAL SCGetProductArea frontier (2026-09-16)

## Hardware blocker

Real Switch hardware produced the next durable direct-dispatch blocker after the merged `OSSetPowerCallback` bridge:

```text
kind                  : DIRECT
target                : 0x801b23a0
guest pc              : 0x800060a4
r1                    : 0x80399118
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_WORKER_READY
action                : abort after durable blocker record
```

This validates the merged PAL `OSSetPowerCallback` (`0x801AB75C`) bridge far enough to expose `SCGetProductArea` (`0x801B23A0`).

## Pinned WiiCompiled semantics

At `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`, `SCGetProductArea_HLE` calls the shared product-region lookup against the SDK table at guest address `0x8029CEB0`, with stride `5` and at most `13` entries. Each row stores the region enum byte followed by a short NUL-terminated area string; `0xFF` terminates the table and no match returns `0xFFFFFFFF`.

The same pin initializes a fresh PAL NAND identity with `AREA=EUR`, `CODE=LEH`, and `GAME=EU`. An existing desktop WiiCompiled NAND `setting.txt` would take precedence, but the Switch fast-track does not yet expose a complete console-identity surface.

## Switch boundary

The Switch bridge therefore mirrors only the observed PAL boundary:

- use the pinned fresh-PAL area `EUR`;
- search the locally translated guest SDK table at `0x8029CEB0` rather than embedding Nintendo table data;
- return the table's matching enum byte in guest `r3`;
- return `0xFFFFFFFF` if the table is absent/unmapped or no row matches;
- do not pre-port `SCGetProductCode`, `SCGetProductSN`, or `SCGetProductGameRegion`.

Public CI remains Nintendo-data-free; its synthetic probe validates only the HLE dispatch seam.
