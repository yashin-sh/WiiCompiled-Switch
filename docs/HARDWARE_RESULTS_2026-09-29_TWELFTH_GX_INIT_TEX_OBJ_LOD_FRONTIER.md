# Hardware result — twelfth exact GXInitTexObjLOD descriptor frontier (2026-09-29)

Tracking: #117

The latest real-Switch run reaches a new exact
`GXInitTexObjLOD (0x80170A4C)` descriptor on an object address seen earlier
in a different format:

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
guest pc         : 0x800060A4
lr               : 0x801814AC
obj              : 0x9018E480
min_filter       : 1
mag_filter       : 1
min/max/bias     : 0 / 0 / 0
bias_clamp       : 0
edge_lod         : 0
max_aniso        : 0
word0            : 0x00000095
word1            : 0x00000000
word2            : 0x0020FC3F
word3            : 0x0080A6F7
word4            : 0x00000000
word5            : 0x00000002
word6            : 0x00000000
word7            : 0x00800202
```

Matching `GXInitTexObj`:

```text
obj       = 0x9018E480
data      = 0x9014DEE0
width     = 64
height    = 64
format    = 2
wrap_s/t  = 1 / 1
mipmap    = 0
blocks    = 128
blockType = 2
flags     = 2
```

This object address was previously hardware-proven with a different exact
format-0 pre-LOD descriptor (`word2=0x0000FC3F`,
`word3=0x0080A890`, `word5=0`, `word7=0x00400102`). Therefore the new
state is a distinct twelfth exact descriptor, not a widening of the prior
fourth descriptor.

## Static caller correlation

The durable blocker now contains `lr=0x801814AC`.

For a normal direct PPC `bl`, `lr - 4 = 0x801814A8`. The previously
generated static GX callsite inventory maps `0x801814A8` to
`GXInitTexObj`, not `GXInitTexObjLOD`.

Public PAL Ghidra metadata places `0x801814A8` inside
`FUN_801813e0` (`0x801813E0..0x8018151F`). The same function contains a
direct `GXInitTexObjLOD` callsite at `0x80181500`.

This gives function-level attribution to `FUN_801813e0`, but the exact LOD
callsite is not claimed as hardware-proven because the LR is stale from the
preceding init call in this HLE sequence.

## Run health

The durable post-main snapshot before the blocker records:

- 34,533 translated / 33,927 post-main dispatches;
- 421 StaticR dispatches;
- 1,408 RMCP01 FIFO writes;
- 92 GXCopyDisp calls;
- 92 successful presents / 0 failures;
- valid FST;
- renderer initialized and frame-active.

Resource reads remain healthy for English.szs, StaticR.rel and Home Button
assets.

The strongest graphics-path invariant observed overall remains 1,450 FIFO
writes and 94 successful presents / 0 failures from earlier runs.

## Minimal implementation

Add only the exact format-2 descriptor above for `obj=0x9018E480` to the
`GXInitTexObjLOD` allowlist.

Do not replace or broaden the already-proven format-0 descriptor for that same
object, and do not add any wrap tuple for this new descriptor without a real
hardware wrap blocker.
