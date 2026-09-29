# Hardware result — tenth GXInitTexObjLOD frontier (2026-09-29)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run reaches a new exact
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x909019C0`.

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
obj              : 0x909019C0
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
word3            : 0x0084635C
word4            : 0x00000000
word5            : 0x00000000
word6            : 0x00000000
word7            : 0x00400102
stage            : RMCP01_GX_INIT_TEX_OBJ_LOD
```

The immediately preceding `GXInitTexObj` completed:

```text
obj       = 0x909019C0
data      = 0x908C6B80
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
word2     = 0x0000FC3F
word3     = 0x0084635C
```

This is the strongest translated-dispatch run so far:

- 62,488 translated / 61,882 durable post-main dispatches;
- 3,683 StaticR dispatches;
- 208 VIWaitForRetrace hits;
- 4,639 post-retrace callbacks;
- 9,321 OSWakeupThread hits;
- 6 TaskThread::run hits;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

The run does not contain explicit pass evidence for the already pending eighth
LOD on `obj=0x908FA820`, ninth LOD on `obj=0x90793BE0`, or sixth wrap on
`obj=0x908FA5C0`; those remain separate scheduler-dependent gates.

## Minimal Switch implementation

Add only the exact tenth pre-LOD descriptor for `obj=0x909019C0` to the
existing allowlist and reuse the already-proven guest mutation / Aurora
`GXInitTexObjLOD` call.

Do not add a corresponding wrap tuple or any neighboring GX texture behavior
until hardware reaches it.

## Hardware acceptance

A later real-Switch run must move durably beyond whichever pending exact GX gate
the scheduler reaches first while preserving resource, scheduler, FIFO,
GXCopyDisp and GPU-present invariants.
