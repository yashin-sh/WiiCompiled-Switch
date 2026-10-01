# Hardware result — tenth GXInitTexObjWrapMode frontier on 0x9018E480 (2026-10-01)

Tracking: #117

The latest real-Switch run reaches the already hardware-proven format-0
`GXInitTexObjLOD` descriptor on `obj=0x9018E480`, records it as
`lod-pass`, then stops at a new exact `GXInitTexObjWrapMode (0x80170B50)`
tuple for that same format-0 state.

```text
GXInitTexObjLOD
status       : lod-pass
obj          : 0x9018E480
guest_word0  : 0x00000195
guest_word1  : 0x00000000

GXInitTexObjWrapMode
status       : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
obj          : 0x9018E480
wrap_s       : 0
wrap_t       : 0
word0_before : 0x00000195
```

Matching init / descriptor:

```text
data      = 0x90151200
size      = 64x64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
word2     = 0x0000FC3F
word3     = 0x0080A890
word5     = 0x00000000
word7     = 0x00400102
```

This is not the separately captured twelfth format-2 descriptor on the same
object. The format-2 descriptor remains an independent pending gate.

## Static caller correlation

The durable blocker records `lr=0x80182C24`, so `lr-4=0x80182C20`.
The static GX callsite inventory does not list `0x80182C20` as the direct
wrap call. It lists the direct `GXInitTexObjWrapMode` callsite at
`0x80182C30`.

Public PAL Ghidra metadata places both addresses in
`FUN_801828dc` (`0x801828DC..0x80183B27`). Therefore the run supports
function-level attribution to `FUN_801828dc`, but the LR is again stale and
is not claimed as the exact wrap callsite.

## Run health

The durable post-main snapshot before the blocker records:

- 34,445 translated / 33,839 post-main dispatches;
- 422 StaticR dispatches;
- 1,408 RMCP01 FIFO writes;
- 92 GXCopyDisp calls;
- 92 successful presents / 0 failures;
- valid FST;
- renderer initialized and frame-active.

Resource reads remain healthy for English.szs, StaticR.rel and Home Button
assets.

The strongest graphics-path evidence remains 1,450 FIFO writes and 94
successful presents / 0 failures from earlier runs.

## Minimal implementation

Add only the exact clamp/clamp wrap tuple for `obj=0x9018E480` requiring the
already-proven format-0 post-LOD descriptor.

Do not authorize a wrap for the separate format-2 descriptor on the same
object and do not widen by object address alone.
