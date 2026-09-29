# Hardware result — seventh GXInitTexObjWrapMode crossed / eleventh GXInitTexObjLOD frontier (2026-09-29)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run records the seventh exact
`GXInitTexObjWrapMode (0x80170B50)` tuple on `obj=0x908FA820` as a
successful `wrap-pass`, then reaches a new exact
`GXInitTexObjLOD (0x80170A4C)` blocker on `obj=0x908FA840`.

Seventh wrap crossing:

```text
status          : wrap-pass
obj             : 0x908FA820
wrap_s          : 0
wrap_t          : 0
word0_before    : 0x00000195
guest_word0     : 0x00000190
guest_word1     : 0x00000000
```

New exact LOD blocker:

```text
kind             : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target           : 0x80170A4C
obj              : 0x908FA840
min_filter       : 1
mag_filter       : 1
min/max/bias     : 0 / 0 / 0
bias_clamp       : 0
edge_lod         : 0
max_aniso        : 0
word0            : 0x00000095
word1            : 0x00000000
word2            : 0x0020FC3F
word3            : 0x00845D15
word4            : 0x00000000
word5            : 0x00000002
word6            : 0x00000000
word7            : 0x00800202
```

The matching completed `GXInitTexObj` is:

```text
obj       = 0x908FA840
data      = 0x908BA2A0
width     = 64
height    = 64
format    = 2
wrap_s/t  = 1 / 1
mipmap    = 0
blocks    = 128
blockType = 2
flags     = 2
```

This is the first captured exact LOD descriptor in the current hardware series
whose pre-LOD descriptor is format 2 rather than format 0. The LOD arguments
remain the same exact linear/linear, zero-LOD, no-clamp/no-edge, GX_ANISO_1
tuple.

## Run health

The run remains healthy through the blocker:

- 61,919 translated / 61,313 post-main dispatches;
- 3,605 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

The strongest translated-dispatch count observed overall remains 62,488 from
the earlier tenth-LOD run.

## Static-caller attribution

This hardware archive does not contain the new durable-blocker `lr` field
added after PR #287. Therefore this run cannot be assigned to one of the
statically enumerated RMCP01 LOD callsites without guessing. Do not infer a
caller from object address or descriptor similarity.

A later build containing PR #287 can correlate a direct PPC call only when the
logged LR matches an enumerated return address, with `callsite = lr - 4`.

## Minimal Switch implementation

Add only the exact `obj=0x908FA840` descriptor above to the
`GXInitTexObjLOD` allowlist.

Do not add a wrap tuple for `0x908FA840` and do not generalize the earlier
format-0 descriptor pattern to this format-2 object.

## Hardware acceptance

A later real-Switch run must move durably beyond whichever pending exact GX
gate the scheduler reaches first while preserving resource, scheduler, FIFO,
GXCopyDisp and GPU-present invariants.
