# Hardware result: GXInitTexObjWrapMode crossed to second GXInitTexObjLOD tuple

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`4ffd4a48dbfbe2979c44f242fe1c0c872bbc5bb2` durably crosses the first
`GXInitTexObjWrapMode (0x80170B50)` candidate:

```text
status=wrap-pass
obj=0x9018e120
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000190
guest_word1=0x00000000
```

The new durable blocker returns to `GXInitTexObjLOD (0x80170A4C)`, but on a
second exact texture object:

```text
kind                  : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target                : 0x80170A4C
obj / r3              : 0x9018E460
min / mag             : 1 / 1
min/max/bias          : 0 / 0 / 0
biasClamp/edge/aniso  : 0 / 0 / 0
word0 / word1         : 0x00000095 / 0x00000000
word2 / word3         : 0x0000FC3F / 0x0080A88F
word4 / word5         : 0x00000000 / 0x00000000
word6 / word7         : 0x00000000 / 0x00400102
```

The immediately preceding GXInitTexObj status identifies the object as:

```text
status=init-pass
obj=0x9018e460
data=0x901511e0
width=64
height=64
format=0
wrap_s=1
wrap_t=1
mipmap=0
```

So this is a 64x64 I4, repeat/repeat, non-mipmapped texture object.

## Decomp attribution

Public `doldecomp/mkw` RMCP01 metadata maps:

```text
GXInitTexObjLOD
PAL: 0x80170A4C..0x80170B50
source: lib/rvl/gx/gxTexture.c
```

The public decomp also shows TPL users calling GXInitTexObj followed by
GXInitTexObjLOD using descriptor-provided filter and LOD values, which matches
this observed sequence.

## Pinned semantics

Pinned WiiCompiled:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

For the observed tuple, pinned `GX__InitTexObjLOD_80170a4c`:

1. reuses the host GXTexObj for the guest object;
2. forwards GX_LINEAR / GX_LINEAR with zero min/max/bias and GX_ANISO_1;
3. mirrors the LOD state into the 32-byte guest GXTexObj.

For this descriptor:

```text
word0: 0x00000095 -> 0x00000195
word1: 0x00000000 -> 0x00000000
```

## Durable progress

The run preserves and slightly extends rendered work:

```text
latest durable dispatch : 35040
latest post-main        : 34434
StaticR dispatches      : 421
VIWaitForRetrace hits   : 197
AsyncDisplay endRender  : 92
GXFlush hits            : 96
GXBegin hits            : 134
RMCP01 FIFO writes      : 1408
GXCopyDisp calls        : 92
present successes       : 92
present failures        : 0
FST structurally valid  : YES
renderer initialized    : YES
renderer frame active   : YES
```

The watchdog remains ACTIVE through its last sample. No native exception or
panic is present.

## Switch candidate

Keep the already proven first LOD tuple and add only this second descriptor:

```text
obj        = 0x9018E460
word0      = 0x00000095
word1      = 0x00000000
word2      = 0x0000FC3F
word3      = 0x0080A88F
word4      = 0x00000000
word5      = 0x00000000
word6      = 0x00000000
word7      = 0x00400102
minFilter  = 1
magFilter  = 1
minLod     = 0
maxLod     = 0
lodBias    = 0
biasClamp  = 0
edgeLod    = 0
maxAniso   = 0
```

The rendered path must reuse the already-created host GXTexObj and apply the
same pinned GXInitTexObjLOD call.

Do not generalize to arbitrary texture objects and do not pre-port neighboring
GX texture functions.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond this second
exact GXInitTexObjLOD tuple while preserving resource, scheduler, FIFO,
GXCopyDisp and present invariants.
