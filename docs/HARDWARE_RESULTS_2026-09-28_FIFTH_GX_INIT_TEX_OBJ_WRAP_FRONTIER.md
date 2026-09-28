# Hardware result — seventh GXInitTexObjLOD crossed / fifth GXInitTexObjWrapMode frontier (2026-09-28)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run uses the main that already contains the
seventh exact `GXInitTexObjLOD (0x80170A4C)` descriptor on
`obj=0x907938A0`. Hardware records that LOD call as a pass and then stops at
the immediately following wrap-mode boundary for the same object.

```text
GXInitTexObjLOD
status          : lod-pass
obj             : 0x907938A0
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
obj              : 0x907938A0
wrap_s           : 0
wrap_t           : 0
word0_before     : 0x00000195
guest_word0      : 0x00000195
guest_word1      : 0x00000000
stage            : RMCP01_GX_INIT_TEX_OBJ_WRAP_MODE
```

The matching `GXInitTexObj` state remains the already captured seventh LOD
descriptor:

```text
obj       = 0x907938A0
data      = 0x90758A60
width     = 64
height    = 64
format    = 0
wrap_s/t  = 1 / 1
mipmap    = 0
word2     = 0x0000FC3F
word3     = 0x0083AC53
```

Runtime invariants remain healthy through the blocker:

- 37,392 translated / 36,786 durable post-main dispatches;
- 1,919 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

This run follows a scheduler path that reaches the seventh LOD + wrap pair
before the already captured eighth LOD on `obj=0x908FA820`. Therefore the
eighth LOD remains a separate merged gate awaiting hardware crossing; this run
does not invalidate or supersede it.

## Exact fifth wrap tuple

The first four accepted wrap tuples are:

```text
0x9018E120 -> clamp/clamp
0x9018E460 -> clamp/clamp
0x9018E140 -> clamp/clamp
0x908FA4E0 -> clamp/clamp
```

Hardware now captures a fifth exact tuple:

```text
0x907938A0 -> clamp/clamp
post-LOD word0 before wrap = 0x00000195
expected word0 after wrap  = 0x00000190
```

## Minimal Switch implementation

Add only `obj=0x907938A0` as the fifth exact wrap tuple and require its
already-proven seventh-LOD descriptor before applying the pinned
`GXInitTexObjWrapMode` mutation.

Do not add a wrap tuple for `obj=0x908FA820` or any neighboring GX texture
behavior until hardware reaches it.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond this fifth wrap tuple
while preserving resource, scheduler, FIFO, GXCopyDisp and GPU-present
invariants. The scheduler may instead reach the already merged eighth LOD gate
first; whichever boundary is observed remains authoritative.
