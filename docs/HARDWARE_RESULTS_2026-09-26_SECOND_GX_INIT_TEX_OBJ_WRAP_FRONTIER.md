# Hardware result: second GXInitTexObjWrapMode tuple

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run from the current runtime line durably crosses the
second exact `GXInitTexObjLOD (0x80170A4C)` descriptor on
`obj=0x9018E460`:

```text
status=lod-pass
obj=0x9018e460
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
r3     : 0x9018E460
r4     : 0x00000000
r5     : 0x00000000
```

The dedicated wrap status captures:

```text
status=GX_INIT_TEX_OBJ_WRAP_MODE_UNPROVEN_TUPLE
obj=0x9018e460
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The immediately preceding `GXInitTexObj` pass identifies the same object as:

```text
obj=0x9018e460
data=0x901511e0
width=64
height=64
format=0
wrap_s=1
wrap_t=1
mipmap=0
word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x0080A88F
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The proven LOD mutation changes only word0/word1 for this exact tuple, so the
post-LOD descriptor entering the wrap call is the observed descriptor above
with word0 `0x00000195` and word1 `0x00000000`.

## Pinned semantics

Pinned WiiCompiled:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

Its `GX__InitTexObjWrapMode_80170b50` implementation:

1. reuses the host `GXTexObj` for the guest object;
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

## Durable progress

The latest durable post-main snapshot remains healthy:

```text
dispatch count         : 33549
post-main dispatch     : 32943
StaticR dispatches     : 421
VIWaitForRetrace hits  : 196
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

The independent heartbeat history stays `ACTIVE` through its final sample.
No `fast-track-exception.txt` was produced.

## Ordering note

This run reaches the second-object wrap boundary before the previously observed
third LOD object `0x9018E140`. That is a scheduler/control-flow ordering
difference, not a regression: the second LOD on `0x9018E460` is explicitly
`lod-pass` in this run.

## Switch candidate

Keep the already proven first wrap tuple on `0x9018E120` and add only this
second exact tuple:

```text
obj        = 0x9018E460
wrapS      = GX_CLAMP
wrapT      = GX_CLAMP
word0      = 0x00000195
word1      = 0x00000000
word2      = 0x0000FC3F
word3      = 0x0080A88F
word4      = 0x00000000
word5      = 0x00000000
word6      = 0x00000000
word7      = 0x00400102
word0After = 0x00000190
```

Do not generalize to arbitrary texture objects or arbitrary wrap modes. Do not
pre-port adjacent texture helpers.

## Hardware acceptance

A later real-Switch run must durably move beyond this second exact wrap tuple
while preserving scheduler, resources, FIFO, GXCopyDisp and present invariants.
