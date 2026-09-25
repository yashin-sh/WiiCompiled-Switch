# Hardware result — IOS_Close crossed / GXLoadTexObj frontier (2026-09-25)

Tracking: #117, #154, #162

## Hardware evidence

The rendered real-Switch run built from merged PR #236 preserves the exact
KD open and first command-2 probe, then records:

```text
status=close-pass
fd=2000
```

This is not treated as sufficient by itself. The same run continues through
later translated work and reaches a distinct unsupported direct boundary, so
`IOS_Close (0x80193AD8)` is hardware-crossed.

The later durable blocker is:

```text
kind   = DIRECT
target = 0x80170F2C
r3     = 0x901136B4
r4     = 0x00000000
stage  = RMCP01_GX_SET_CHAN_CTRL
```

## Deeper game/resource progress

The same run advances beyond the prior boot-resource-only path:

- `RKSystem::run hits = 1`;
- `TaskThread::run hits = 2`;
- local DVD read of `/rel/StaticR.rel` succeeds with 4,903,876 bytes;
- scheduler/message activity remains live;
- the independent watchdog remains ACTIVE before the terminal blocker.

This is the first hardware-proven `StaticR.rel` read in the current path.

## Preserved render invariants

The previously established graphics path remains healthy:

- FST remains published at `0x97DC0000`, 64,224 bytes / 2,096 entries;
- `English.szs` remains read-pass at 299,969 bytes;
- `GXInitTexObj` remains init-pass at 832x456, format 4;
- 29 RMCP01 FIFO writes are observed;
- FIFO produced work remains YES;
- `GXCopyDisp calls = 1`;
- one successful present and zero present failures.

This preserves the GPU-present proof but still does not claim visually correct
Mario Kart Wii pixels.

## Pinned mapping

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`
maps `0x80170F2C` exactly to:

```cpp
GX__LoadTexObj_80170f2c(uint32_t oa, uint32_t tid)
```

with:

```text
oa  = r3 = 0x901136B4
tid = r4 = 0
```

The pinned implementation first resolves metadata with
`TryGetOrExtractTexObjMeta(oa, meta)`. If no cached slot exists, it decodes
the 32-byte guest GXTexObj directly. Those bytes contain the texture backing
address, width, height, format, wrap modes, mipmap state and LOD/filter fields.

The live object `0x901136B4` is distinct from the previously captured
`GXInitTexObj` object `0x901136D4`. Therefore that prior init-pass record
cannot be reused as metadata for this load.

Pinned GXLoadTexObj behavior can also diverge for invalid/unsupported formats
and CI/TLUT textures. No such format is attributed until hardware captures the
actual guest object.

## Diagnostic-only candidate

The current candidate changes no GX load behavior.

For blocker target `0x80170F2C`, it extends
`fast-track-dispatch-blocker.txt` with:

1. `oa` and `tid`;
2. whether the full 0x20-byte guest object is readable;
3. all eight 32-bit GXTexObj words;
4. directly decoded backing address;
5. width and height;
6. format from guest word5 and the fallback format nibble from word2;
7. wrap S/T;
8. mipmap flag.

These fields follow the pinned guest-structure decoding contract and do not
fabricate texture metadata.

No `GXLoadTexObj`, `GXInitTexObjLOD`, CI/TLUT, texture invalidation, or
neighboring GX function is ported in this candidate.

## Next hardware acceptance

After merge with all five public CI gates green, the next private rendered run
must reproduce the `0x80170F2C` blocker and provide the extended texture
fields.

Only then should the exact observed format/data/dimensions choose the minimal
GXLoadTexObj implementation. If the blocker moves unexpectedly, the new durable
hardware result takes precedence.
