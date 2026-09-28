# Hardware result — seventh GXInitTexObjLOD crossed / eighth GXInitTexObjLOD frontier (2026-09-28)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged seventh exact
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x907938A0`
moves durably beyond the previous blocker and reaches a new exact LOD tuple.

The new durable blocker is:

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
obj              : 0x908FA820
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
word3            : 0x00845EAD
word4            : 0x00000000
word5            : 0x00000000
word6            : 0x00000000
word7            : 0x00400102
stage            : RMCP01_GX_INIT_TEX_OBJ_LOD
```

The immediately preceding `GXInitTexObj` for the same object completed:

```text
obj       = 0x908FA820
data      = 0x908BD5A0
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
```

Runtime invariants remain healthy through the blocker:

- 55,165 translated / 54,559 durable post-main dispatches;
- 3,604 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

The prior run stopped on the seventh exact descriptor at `obj=0x907938A0`
around 36,465 translated dispatches. This run reaches a later distinct LOD
boundary at 55,165 translated dispatches with the seventh tuple already
allowed by the merged bridge, so the seventh gate is accepted as
hardware-crossed for the fast-track progression.

## Exact eighth LOD tuple

The new descriptor is distinct from all seven prior captured descriptors:

```text
0x9018E120 -> word3 0x0080A997
0x9018E460 -> word3 0x0080A88F
0x9018E140 -> word3 0x0080A998
0x9018E480 -> word3 0x0080A890
0x908FA4E0 -> word3 0x00845FB5
0x908FA5C0 -> word3 0x00845FBC
0x907938A0 -> word3 0x0083AC53
0x908FA820 -> word3 0x00845EAD   <-- new hardware tuple
```

The call arguments remain the already-proven pinned
`GXInitTexObjLOD` shape: linear/linear filtering, zero min/max LOD and bias,
no bias clamp, no edge LOD, GX_ANISO_1.

## Minimal Switch implementation

Add only the exact eighth pre-LOD descriptor for `obj=0x908FA820` to the
existing allowlist and reuse the already-proven guest mutation / Aurora
`GXInitTexObjLOD` call.

Do not add a corresponding `GXInitTexObjWrapMode` tuple or any neighboring GX
texture behavior until hardware reaches it.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond this eighth
`GXInitTexObjLOD` tuple while preserving resource, scheduler, FIFO,
GXCopyDisp and GPU-present invariants.
