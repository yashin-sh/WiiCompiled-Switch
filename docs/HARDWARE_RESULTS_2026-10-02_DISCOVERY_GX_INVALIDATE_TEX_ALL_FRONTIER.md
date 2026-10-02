# Hardware result — Discovery scan / GXInvalidateTexAll frontier (2026-10-02)

The first rendered Discovery NRO run reaches a new PAL GX texture boundary
after the earlier texture-object work has already continued successfully.

## Durable blocker

```text
kind                  : DIRECT
target                : 0x80171110
lr                    : 0x80240e54
fast-track stage      : RMCP01_GX_INIT_TEX_OBJ_LOD
```

Public PAL attribution identifies `0x80171110` as `GXInvalidateTexAll`.
Pinned WiiCompiled implements the same entry point as a no-argument native
override that forwards directly to Aurora `GXInvalidateTexAll()`.

## Discovery run health

The same hardware run records:

- 604,194 translated dispatches at the blocker;
- 1,236 unique first-hit runtime targets;
- PAL main reached;
- 3,826 StaticR dispatches;
- 1,555 RMCP01 FIFO writes;
- 99 `GXCopyDisp` calls;
- 99 successful presents;
- zero present failures;
- structurally valid FST at `0x97DC0000`.

The final observed texture state immediately before the blocker includes a
4x4 format-3 object at `0x80384500` and a successful LOD pass. This frontier
is therefore later than the previously repeated texture-descriptor gates.

## Port decision

Map only PAL `GXInvalidateTexAll (0x80171110)` to the pinned no-argument
Aurora call. No predictive neighboring GX functions are included in this
change.
