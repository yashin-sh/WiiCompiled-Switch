# Hardware result — AIStartDMA and third wrap crossed / fourth GXInitTexObjLOD frontier (2026-09-27)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged
`AIStartDMA (0x80124048)` bridge moves durably beyond that audio boundary.

The same run also records:

```text
status=wrap-pass
obj=0x9018E140
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000190
guest_word1=0x00000000
```

so the third exact `GXInitTexObjWrapMode (0x80170B50)` tuple is now
hardware-crossed.

The new exact durable blocker is:

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
obj              : 0x9018E480
min_filter       : 1
mag_filter       : 1
min_lod_bits     : 0x00000000
max_lod_bits     : 0x00000000
lod_bias_bits    : 0x00000000
bias_clamp       : 0
edge_lod         : 0
max_aniso        : 0
word0            : 0x00000095
word1            : 0x00000000
word2            : 0x0000FC3F
word3            : 0x0080A890
word4            : 0x00000000
word5            : 0x00000000
word6            : 0x00000000
word7            : 0x00400102
stage            : RMCP01_GX_INIT_TEX_OBJ_LOD
```

Runtime invariants remain healthy through the blocker:

- strongest durable snapshot: 31,175 translated / 30,569 post-main dispatches;
- 422 StaticR dispatches;
- 1,408 RMCP01 FIFO writes;
- 92 GXCopyDisp calls;
- 92 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- English.szs, StaticR.rel and Home Button/UI reads preserved;
- SZS decode PASS.

## Exact fourth LOD tuple

This is not any of the three already-proven descriptors:

```text
0x9018E120 -> word3 0x0080A997
0x9018E460 -> word3 0x0080A88F
0x9018E140 -> word3 0x0080A998
0x9018E480 -> word3 0x0080A890   <-- new hardware tuple
```

The call arguments are otherwise the same already-proven pinned
`GXInitTexObjLOD` shape: linear/linear filtering, zero min/max LOD and bias,
no bias clamp, no edge LOD, and GX_ANISO_1.

## Minimal Switch implementation

Add only the exact fourth descriptor `obj=0x9018E480` with the captured
pre-LOD words to the existing `GXInitTexObjLOD` allowlist.

Reuse the already-proven pinned LOD guest mutation and Aurora call.

Do not add a fourth `GXInitTexObjWrapMode` tuple yet. If hardware reaches a
wrap call for `0x9018E480`, that exact tuple becomes the next frontier.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond this fourth
`GXInitTexObjLOD` tuple while preserving resource, scheduler, FIFO,
GXCopyDisp and present invariants.
