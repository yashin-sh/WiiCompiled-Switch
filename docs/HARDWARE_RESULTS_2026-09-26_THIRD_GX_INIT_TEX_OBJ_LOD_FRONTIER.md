# Hardware result: second GXInitTexObjLOD tuple crossed to third tuple

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`3c3630d4580fb17a20d690786bca39063b0f69b1` durably moves beyond the
previous second `GXInitTexObjLOD (0x80170A4C)` descriptor on
`obj=0x9018E460`.

The new durable blocker is another invocation of the same PAL function:

```text
kind                  : GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE
target                : 0x80170A4C
obj / r3              : 0x9018E140
min / mag             : 1 / 1
min/max/bias          : 0 / 0 / 0
biasClamp/edge/aniso  : 0 / 0 / 0
word0 / word1         : 0x00000095 / 0x00000000
word2 / word3         : 0x0000FC3F / 0x0080A998
word4 / word5         : 0x00000000 / 0x00000000
word6 / word7         : 0x00000000 / 0x00400102
```

The immediately preceding `GXInitTexObj` status identifies this object as:

```text
status=init-pass
obj=0x9018E140
data=0x90153300
width=64
height=64
format=0
wrap_s=1
wrap_t=1
mipmap=0
```

So this is another 64x64 I4, repeat/repeat, non-mipmapped texture object.

## Decomp attribution

Public `doldecomp/mkw` RMCP01 metadata maps:

```text
GXInitTexObjLOD
PAL: 0x80170A4C..0x80170B50
source: lib/rvl/gx/gxTexture.c
```

The public TPL code also shows the expected sequence of `GXInitTexObj`
followed by `GXInitTexObjLOD`, matching the observed hardware path.

## Pinned semantics

Pinned WiiCompiled:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

For the exact observed linear/linear zero-LOD tuple, the pinned implementation
reuses the host GXTexObj, forwards the LOD state to Aurora, then mirrors it
into guest memory.

For this descriptor:

```text
word0: 0x00000095 -> 0x00000195
word1: 0x00000000 -> 0x00000000
```

## Durable progress

The latest durable post-main snapshot remains healthy:

```text
dispatch count         : 31949
post-main dispatch     : 31343
StaticR dispatches     : 422
VIWaitForRetrace hits  : 197
AsyncDisplay endRender : 92
GXFlush hits           : 96
GXBegin hits           : 134
RMCP01 FIFO writes     : 1408
GXCopyDisp calls       : 92
present successes      : 92
present failures       : 0
FST structurally valid : YES
renderer initialized   : YES
renderer frame active  : YES
```

The independent watchdog stays ACTIVE through its final sample. No native
exception or panic file is present.

## Switch candidate

Keep the already proven first and second LOD descriptors and add only this
third exact descriptor:

```text
obj        = 0x9018E140
word0      = 0x00000095
word1      = 0x00000000
word2      = 0x0000FC3F
word3      = 0x0080A998
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

The rendered path must reuse the host object already created by the immediately
preceding GXInitTexObj and apply the same pinned GXInitTexObjLOD call.

Do not generalize to arbitrary texture objects and do not pre-port neighboring
GX texture functions.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond this third exact
GXInitTexObjLOD tuple while preserving resource, scheduler, FIFO, GXCopyDisp
and present invariants.
