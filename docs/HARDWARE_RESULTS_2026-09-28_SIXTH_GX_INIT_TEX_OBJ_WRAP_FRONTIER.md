# Hardware result — sixth GXInitTexObjWrapMode frontier (2026-09-28)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run uses a main that already contains the
merged eighth exact `GXInitTexObjLOD` candidate and fifth exact
`GXInitTexObjWrapMode` candidate, but this scheduler path reaches neither
pending gate first.

Hardware records the sixth exact LOD descriptor on `obj=0x908FA5C0` as a
successful `lod-pass` and then stops at the immediately following unproven
wrap-mode tuple for that same object.

```text
GXInitTexObjLOD
status          : lod-pass
obj             : 0x908FA5C0
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

The new exact durable blocker is:

```text
kind             : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
target           : 0x80170B50
obj              : 0x908FA5C0
wrap_s           : 0
wrap_t           : 0
word0_before     : 0x00000195
guest_word0      : 0x00000195
guest_word1      : 0x00000000
stage            : RMCP01_GX_INIT_TEX_OBJ_WRAP_MODE
```

The matching `GXInitTexObj` descriptor remains:

```text
obj       = 0x908FA5C0
data      = 0x908BF780
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
word2     = 0x0000FC3F
word3     = 0x00845FBC
```

Runtime invariants remain healthy through the blocker:

- 59,540 translated / 58,934 durable post-main dispatches;
- 3,608 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

This run does not prove durable crossing of the separate pending fifth wrap on
`obj=0x907938A0` or the eighth LOD on `obj=0x908FA820`; both remain
scheduler-dependent hardware gates.

## Exact sixth wrap tuple

The previously captured wrap tuples are:

```text
0x9018E120 -> clamp/clamp
0x9018E460 -> clamp/clamp
0x9018E140 -> clamp/clamp
0x908FA4E0 -> clamp/clamp
0x907938A0 -> clamp/clamp
```

Hardware now captures a sixth exact tuple:

```text
0x908FA5C0 -> clamp/clamp
post-LOD word0 before wrap = 0x00000195
expected word0 after wrap  = 0x00000190
```

## Minimal Switch implementation

Add only `obj=0x908FA5C0` as the sixth exact wrap tuple and require its
already-proven sixth-LOD descriptor before applying the pinned
`GXInitTexObjWrapMode` mutation.

Do not alter the pending fifth wrap on `obj=0x907938A0`, do not add a wrap
tuple for `obj=0x908FA820`, and do not pre-port neighboring GX texture
behavior.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond whichever pending
scheduler-dependent GX gate it reaches first:

- eighth `GXInitTexObjLOD` on `obj=0x908FA820`;
- fifth `GXInitTexObjWrapMode` on `obj=0x907938A0`;
- sixth `GXInitTexObjWrapMode` on `obj=0x908FA5C0`.

Resource, scheduler, FIFO, GXCopyDisp and GPU-present invariants must remain
healthy.
