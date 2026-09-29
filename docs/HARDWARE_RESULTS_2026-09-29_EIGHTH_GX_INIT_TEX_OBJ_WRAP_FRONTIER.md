# Hardware result — tenth GXInitTexObjLOD crossed / eighth GXInitTexObjWrapMode frontier (2026-09-29)

Tracking: #117

The latest real-Switch run records the tenth exact `GXInitTexObjLOD` on
`obj=0x909019C0` as `lod-pass`, then stops at an exact unproven
`GXInitTexObjWrapMode` tuple for the same object.

```text
GXInitTexObjLOD
status       : lod-pass
obj          : 0x909019C0
guest_word0  : 0x00000195
guest_word1  : 0x00000000

GXInitTexObjWrapMode
status       : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
obj          : 0x909019C0
wrap_s       : 0
wrap_t       : 0
word0_before : 0x00000195
```

Matching pre-LOD descriptor:

```text
word0 = 0x00000095
word1 = 0x00000000
word2 = 0x0000FC3F
word3 = 0x0084635C
word4 = 0x00000000
word5 = 0x00000000
word6 = 0x00000000
word7 = 0x00400102
```

Matching `GXInitTexObj`: data `0x908C6B80`, 64x64, format 0, wrap 1/1,
no mipmap.

Run health remains intact: 60,652 translated / 60,046 post-main dispatches,
3,683 StaticR dispatches, 1,450 FIFO writes, 94 GXCopyDisp calls, 94 successful
presents / 0 failures, valid FST, renderer initialized and frame-active.

The blocker archive does not expose a usable LR at the wrap stop, so no static
caller is inferred for this run.

Minimal implementation: add only the exact clamp/clamp wrap tuple for
`obj=0x909019C0`, requiring the already observed tenth-LOD descriptor. No
neighboring GX texture behavior is pre-ported.
