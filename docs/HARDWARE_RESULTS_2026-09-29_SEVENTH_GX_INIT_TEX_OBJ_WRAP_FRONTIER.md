# Hardware result — eighth GXInitTexObjLOD crossed / seventh GXInitTexObjWrapMode frontier (2026-09-29)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run records the eighth exact
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x908FA820` as a
successful `lod-pass`, then stops at the immediately following unproven
`GXInitTexObjWrapMode (0x80170B50)` tuple for that same object.

```text
GXInitTexObjLOD
status          : lod-pass
obj             : 0x908FA820
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
obj              : 0x908FA820
wrap_s           : 0
wrap_t           : 0
word0_before     : 0x00000195
guest_word0      : 0x00000195
guest_word1      : 0x00000000
stage            : RMCP01_GX_INIT_TEX_OBJ_WRAP_MODE
```

The matching completed `GXInitTexObj` state is:

```text
obj       = 0x908FA820
data      = 0x908BD5A0
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
word2     = 0x0000FC3F
word3     = 0x00845EAD
```

Runtime invariants remain healthy through the blocker:

- 62,162 translated / 61,556 durable post-main dispatches;
- 3,605 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

This run explicitly hardware-crosses the eighth LOD gate. It does not contain
explicit pass evidence for the separate pending ninth LOD on `0x90793BE0`,
tenth LOD on `0x909019C0`, or sixth wrap on `0x908FA5C0`.

## Minimal Switch implementation

Add only `obj=0x908FA820` as the seventh exact wrap tuple and require the
already-proven eighth-LOD descriptor before applying the pinned
`GXInitTexObjWrapMode` mutation.

Do not add wrap tuples for `0x90793BE0` or `0x909019C0`, and do not
pre-port neighboring GX texture behavior.

## Hardware acceptance

A later real-Switch run must move durably beyond whichever pending exact GX
gate the scheduler reaches first while preserving resource, scheduler, FIFO,
GXCopyDisp and GPU-present invariants.
