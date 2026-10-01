# Hardware result — thirteenth GXInitTexObjLOD crossed / eleventh GXInitTexObjWrapMode frontier (2026-10-01)

Tracking: #117

The latest real-Switch run records the thirteenth exact
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x908FAE00` as a
successful `lod-pass`, then reaches a new exact
`GXInitTexObjWrapMode (0x80170B50)` blocker on the same object.

Thirteenth LOD crossing:

```text
status          : lod-pass
obj             : 0x908FAE00
min_filter      : 1
mag_filter      : 1
min_lod_bits    : 0x00000000
max_lod_bits    : 0x00000000
lod_bias_bits   : 0x00000000
bias_clamp      : 0
edge_lod        : 0
max_aniso       : 0
guest_word0     : 0x00000195
guest_word1     : 0x00000000
```

New exact wrap blocker:

```text
kind             : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
target           : 0x80170B50
guest pc         : 0x800060A4
lr               : 0x80182C24
obj              : 0x908FAE00
wrap_s           : 0
wrap_t           : 0
word0_before     : 0x00000195
guest_word0      : 0x00000195
guest_word1      : 0x00000000
```

Matching exact descriptor:

```text
word0 = 0x00000095
word1 = 0x00000000
word2 = 0x0000FC3F
word3 = 0x00845FB5
word4 = 0x00000000
word5 = 0x00000000
word6 = 0x00000000
word7 = 0x00400102
```

Matching `GXInitTexObj`: data `0x908BF6A0`, 64x64, format 0,
wrap repeat/repeat, no mipmap.

## Static caller correlation

The wrap blocker records `lr=0x80182C24`, so `lr-4=0x80182C20`.
The static GX callsite inventory instead lists the direct
`GXInitTexObjWrapMode` call at `0x80182C30`.

Public PAL Ghidra metadata places the surrounding code in
`FUN_801828dc` (`0x801828DC..0x80183B27`). This supports function-level
attribution only; the logged LR is stale and is not claimed as the exact wrap
callsite.

## Run health

The durable post-main snapshot before the blocker records:

- 55,825 translated / 55,219 post-main dispatches;
- 3,604 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful presents / 0 failures;
- valid FST;
- renderer initialized and frame-active.

Resource reads remain healthy for English.szs, StaticR.rel,
`revo_kart.brsar` and Home Button assets.

The strongest translated-dispatch count observed overall remains 62,488 from
an earlier scheduler path.

## Minimal implementation

Add only the exact clamp/clamp wrap tuple for `obj=0x908FAE00`, requiring
the already hardware-proven thirteenth-LOD descriptor.

Do not widen by descriptor contents alone and do not alter neighboring GX
texture behavior.
