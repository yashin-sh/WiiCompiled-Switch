# Hardware result — eleventh GXInitTexObjLOD crossed / ninth GXInitTexObjWrapMode frontier (2026-09-29)

Tracking: #117

The latest real-Switch run records the eleventh exact `GXInitTexObjLOD` on
`obj=0x908FA840` as `lod-pass`, then stops at an exact unproven
`GXInitTexObjWrapMode` tuple for the same object.

```text
GXInitTexObjLOD
status       : lod-pass
obj          : 0x908FA840
guest_word0  : 0x00000195
guest_word1  : 0x00000000

GXInitTexObjWrapMode
status       : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
obj          : 0x908FA840
wrap_s       : 0
wrap_t       : 0
word0_before : 0x00000195
```

Matching pre-LOD descriptor:

```text
word0 = 0x00000095
word1 = 0x00000000
word2 = 0x0020FC3F
word3 = 0x00845D15
word4 = 0x00000000
word5 = 0x00000002
word6 = 0x00000000
word7 = 0x00800202
```

Matching `GXInitTexObj`: data `0x908BA2A0`, 64x64, format 2, wrap 1/1,
no mipmap.

Run health remains intact:

- 56,503 translated / 55,897 post-main dispatches;
- 3,604 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful presents / 0 failures;
- valid FST;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

The strongest translated-dispatch count observed overall remains 62,488 from
the earlier tenth-LOD run.

The blocker record still lacks a guest LR because PR #287 added LR to durable
liveness snapshots rather than the unsupported-dispatch formatter. Caller
attribution therefore remains unavailable for this archive; a separate
diagnostic correction is required.

Minimal implementation: add only the exact clamp/clamp wrap tuple for
`obj=0x908FA840`, requiring the already hardware-proven eleventh-LOD
descriptor. No neighboring GX texture behavior is pre-ported.
