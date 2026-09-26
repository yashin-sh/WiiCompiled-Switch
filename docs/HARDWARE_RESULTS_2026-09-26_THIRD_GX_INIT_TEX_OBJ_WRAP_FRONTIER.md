# Hardware result: third GXInitTexObjWrapMode tuple

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`f57d6c19e50bfdb8c02151f73aae63a1e4b51e6a` durably moves beyond the
previous second GX texture wrap frontier.

The third exact `GXInitTexObjLOD (0x80170A4C)` descriptor on
`obj=0x9018E140` is now explicitly crossed:

```text
status=lod-pass
obj=0x9018e140
min_filter=1
mag_filter=1
min_lod_bits=0x00000000
max_lod_bits=0x00000000
lod_bias_bits=0x00000000
bias_clamp=0
edge_lod=0
max_aniso=0
guest_word0=0x00000195
guest_word1=0x00000000
```

The new durable blocker is:

```text
kind   : GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
target : 0x80170B50
r3     : 0x9018E140
r4     : 0x00000000
r5     : 0x00000000
```

The dedicated wrap status captures:

```text
status=GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
obj=0x9018e140
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The immediately preceding `GXInitTexObj` pass identifies the same object as:

```text
obj=0x9018e140
data=0x90153300
width=64
height=64
format=0
wrap_s=1
wrap_t=1
mipmap=0
word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x0080A998
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

## Decomp attribution

Public `doldecomp/mkw` RMCP01 metadata maps:

```text
GXInitTexObjWrapMode
PAL: 0x80170B50..0x80170B64
source: lib/rvl/gx/gxTexture.c
```

## Pinned semantics

Pinned WiiCompiled:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

Its `GX__InitTexObjWrapMode_80170b50` implementation:

1. reuses the host GXTexObj for the guest object;
2. records wrap S/T metadata;
3. mutates guest word0 with
   `(word0 & ~0xF) | (wrapS & 3) | ((wrapT & 3) << 2)`;
4. forwards the same modes to Aurora `GXInitTexObjWrapMode`.

For this hardware tuple:

```text
word0: 0x00000195 -> 0x00000190
word1: 0x00000000 -> 0x00000000
wrapS/wrapT: GX_CLAMP / GX_CLAMP
```

## Durable runtime snapshot

The latest durable post-main snapshot remains healthy:

```text
dispatch count         : 31181
post-main dispatch     : 30575
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

The independent watchdog remains ACTIVE through the final recorded sample.
No native exception file is present.

## Crossing implication

Reaching this third-object wrap proves both earlier exact wrap tuples are
behind the durable frontier for this execution path. In particular, the
second wrap on `obj=0x9018E460` no longer blocks execution.

## Switch candidate

Keep the already proven first and second wrap tuples and add only this third
exact post-LOD descriptor:

```text
obj        = 0x9018E140
wrapS      = GX_CLAMP
wrapT      = GX_CLAMP
word0      = 0x00000195
word1      = 0x00000000
word2      = 0x0000FC3F
word3      = 0x0080A998
word4      = 0x00000000
word5      = 0x00000000
word6      = 0x00000000
word7      = 0x00400102
word0After = 0x00000190
```

Do not generalize to arbitrary texture objects or arbitrary wrap modes. Do not
pre-port adjacent texture helpers.

## Hardware acceptance

A later real-Switch run must durably move beyond this third exact wrap tuple
while preserving scheduler, resource, FIFO, GXCopyDisp and present invariants.
