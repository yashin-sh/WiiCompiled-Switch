# Hardware result — ninth GXInitTexObjWrapMode crossed / thirteenth GXInitTexObjLOD frontier (2026-09-30)

Tracking: #117

The latest real-Switch run records the ninth exact
`GXInitTexObjWrapMode (0x80170B50)` tuple on `obj=0x908FA840` as a
successful `wrap-pass`, then reaches a new exact
`GXInitTexObjLOD (0x80170A4C)` blocker on `obj=0x908FAE00`.

Ninth wrap crossing:

```text
status          : wrap-pass
obj             : 0x908FA840
wrap_s          : 0
wrap_t          : 0
word0_before    : 0x00000195
guest_word0     : 0x00000190
guest_word1     : 0x00000000
```

New exact LOD blocker:

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
guest pc         : 0x800060A4
lr               : 0x801814AC
obj              : 0x908FAE00
min_filter       : 1
mag_filter       : 1
min/max/bias     : 0 / 0 / 0
bias_clamp       : 0
edge_lod         : 0
max_aniso        : 0
word0            : 0x00000095
word1            : 0x00000000
word2            : 0x0000FC3F
word3            : 0x00845FB5
word4            : 0x00000000
word5            : 0x00000000
word6            : 0x00000000
word7            : 0x00400102
```

Matching completed `GXInitTexObj`:

```text
obj       = 0x908FAE00
data      = 0x908BF6A0
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
blocks    = 64
blockType = 1
flags     = 2
```

The descriptor words are byte-for-byte identical to the previously proven
fifth LOD descriptor on `obj=0x908FA4E0`, but the object address is
different. The bridge therefore treats `obj=0x908FAE00` as a separate exact
descriptor instead of widening by descriptor contents alone.

## Static caller correlation

The durable blocker again records `lr=0x801814AC`. Therefore
`lr-4=0x801814A8`, which the static GX callsite inventory maps to
`GXInitTexObj`, not `GXInitTexObjLOD`.

Public PAL Ghidra metadata places that call in `FUN_801813e0`
(`0x801813E0..0x8018151F`), which also contains the direct
`GXInitTexObjLOD` callsite at `0x80181500`. As with the previous format-2
run, this supports function-level attribution only; the logged LR is stale
from the preceding init call and is not claimed as the exact LOD callsite.

## Run health

The durable post-main snapshot before the blocker records:

- 60,683 translated / 60,077 post-main dispatches;
- 3,605 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful presents / 0 failures;
- valid FST;
- renderer initialized and frame-active.

Resource reads remain healthy for English.szs, StaticR.rel, revo_kart.brsar
and Home Button assets.

The strongest translated-dispatch count observed overall remains 62,488 from
an earlier scheduler path.

## Minimal implementation

Add only the exact `obj=0x908FAE00` descriptor above to the
`GXInitTexObjLOD` allowlist.

Do not generalize the earlier `word3=0x00845FB5` descriptor across objects
and do not add a wrap tuple for `0x908FAE00` without a later real-hardware
wrap blocker.
