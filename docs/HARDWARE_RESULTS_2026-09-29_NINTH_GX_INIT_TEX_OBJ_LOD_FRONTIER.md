# Hardware result — fifth GXInitTexObjWrapMode crossed / ninth GXInitTexObjLOD frontier (2026-09-29)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run records the fifth exact
`GXInitTexObjWrapMode (0x80170B50)` tuple on `obj=0x907938A0` as a
successful `wrap-pass`, then reaches a new exact
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x90793BE0`.

The proven fifth wrap pass is:

```text
status       : wrap-pass
obj          : 0x907938A0
wrap_s       : 0
wrap_t       : 0
word0_before : 0x00000195
guest_word0  : 0x00000190
guest_word1  : 0x00000000
```

The new exact durable blocker is:

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
obj              : 0x90793BE0
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
word3            : 0x0083AB4B
word4            : 0x00000000
word5            : 0x00000000
word6            : 0x00000000
word7            : 0x00400102
stage            : RMCP01_GX_INIT_TEX_OBJ_LOD
```

The immediately preceding `GXInitTexObj` for the same object completed:

```text
obj       = 0x90793BE0
data      = 0x90756960
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
word2     = 0x0000FC3F
word3     = 0x0083AB4B
```

Runtime invariants remain healthy through the blocker:

- 35,691 translated / 35,085 durable post-main dispatches;
- 1,919 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

This run does **not** prove durable crossing of the separate pending eighth
LOD on `obj=0x908FA820` or sixth wrap on `obj=0x908FA5C0`; both remain
scheduler-dependent hardware gates.

## Exact ninth LOD tuple

The ninth captured descriptor is distinct from all prior exact descriptors:

```text
0x9018E120 -> word3 0x0080A997
0x9018E460 -> word3 0x0080A88F
0x9018E140 -> word3 0x0080A998
0x9018E480 -> word3 0x0080A890
0x908FA4E0 -> word3 0x00845FB5
0x908FA5C0 -> word3 0x00845FBC
0x907938A0 -> word3 0x0083AC53
0x908FA820 -> word3 0x00845EAD
0x90793BE0 -> word3 0x0083AB4B   <-- new hardware tuple
```

The call arguments remain the already-proven pinned
`GXInitTexObjLOD` shape: GX_LINEAR/GX_LINEAR, zero min/max LOD and bias,
no bias clamp, no edge LOD, GX_ANISO_1.

## Minimal Switch implementation

Add only the exact ninth pre-LOD descriptor for `obj=0x90793BE0` to the
existing allowlist and reuse the already-proven guest mutation / Aurora
`GXInitTexObjLOD` call.

Do not alter the pending eighth LOD or sixth wrap gates, and do not pre-port a
wrap tuple for `obj=0x90793BE0` or any neighboring GX texture behavior.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond whichever pending
scheduler-dependent GX gate it reaches first:

- eighth `GXInitTexObjLOD` on `obj=0x908FA820`;
- sixth `GXInitTexObjWrapMode` on `obj=0x908FA5C0`;
- ninth `GXInitTexObjLOD` on `obj=0x90793BE0`.

Resource, scheduler, FIFO, GXCopyDisp and GPU-present invariants must remain
healthy.
