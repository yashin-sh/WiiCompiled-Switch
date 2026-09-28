# Hardware result — fifth GXInitTexObjLOD crossed / fourth GXInitTexObjWrapMode frontier (2026-09-28)

Tracking: #117

## Real-Switch evidence

The supplied rendered fast-track run built after the merged fifth
`GXInitTexObjLOD (0x80170A4C)` descriptor on `obj=0x908FA4E0`
moves durably beyond that LOD boundary.

The new exact durable blocker is:

```text
kind             : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
target           : 0x80170B50
obj              : 0x908FA4E0
wrap_s           : 0
wrap_t           : 0
word0_before     : 0x00000195
guest_word0      : 0x00000195
guest_word1      : 0x00000000
stage            : RMCP01_GX_INIT_TEX_OBJ_WRAP_MODE
```

The exact post-LOD descriptor for this object remains:

```text
word0 = 0x00000195
word1 = 0x00000000
word2 = 0x0000FC3F
word3 = 0x00845FB5
word4 = 0x00000000
word5 = 0x00000000
word6 = 0x00000000
word7 = 0x00400102
```

Runtime invariants remain healthy through the blocker:

- 63,314 translated / 62,708 post-main dispatches;
- 3,605 StaticR dispatches;
- 1,450 RMCP01 FIFO writes;
- 94 GXCopyDisp calls;
- 94 successful GPU presents / 0 failures;
- FST structurally valid;
- renderer initialized and frame-active;
- real `revo_kart.brsar` and Home Button resource reads preserved.

This proves the fifth exact `GXInitTexObjLOD` descriptor is hardware-crossed.

## Minimal Switch implementation

Add only the exact fourth `GXInitTexObjWrapMode` tuple:

```text
obj   = 0x908FA4E0
wrapS = GX_CLAMP
wrapT = GX_CLAMP
```

Require the exact fifth-descriptor post-LOD state before applying the already
proven pinned guest/Aurora wrap mutation.

Do not add any further texture object, load, wrap or LOD variation until
hardware reaches it.

## Hardware acceptance

A later rendered real-Switch run must move durably beyond this fourth exact
`GXInitTexObjWrapMode` tuple while preserving resource, scheduler, FIFO,
GXCopyDisp and GPU-present invariants.
