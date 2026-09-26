# Hardware result: GXInitTexObjLOD crossed to GXInitTexObjWrapMode

Date: 2026-09-26

## Hardware evidence

A rendered real-Switch run built from merged main
`2e11793e1359a227a81f0867e47fe5b24f863cb8` durably moves beyond the
previous `GXInitTexObjLOD (0x80170A4C)` frontier.

The LOD bridge reports:

```text
status=lod-pass
obj=0x9018e120
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

The durable unsupported dispatch is now:

```text
kind                  : DIRECT
target                : 0x80170B50
r3 / obj              : 0x9018E120
r4 / wrapS            : 0
r5 / wrapT            : 0
stage                 : RMCP01_GX_INIT_TEX_OBJ_LOD
```

Public `doldecomp/mkw` RMCP01 metadata maps
`0x80170B50..0x80170B64` to `GXInitTexObjWrapMode`.

## Durable progress

The same run improves the rendered counters while preserving all major
invariants:

```text
dispatch count         : 33485
post-main dispatch     : 32879
StaticR dispatches     : 421
VIWaitForRetrace hits  : 196
PostRetrace cb hits    : 2931
TaskThread::run hits   : 3
GXBegin hits           : 133
GXFlush hits           : 95
RMCP01 FIFO writes     : 1387
GXCopyDisp calls       : 91
present successes      : 91
present failures       : 0
FST structurally valid : YES
renderer initialized   : YES
renderer frame active  : YES
```

This advances the previous 84-present plateau to 91 successful presents with
zero failures.

Home Button/UI resource reads remain successful, including
`HomeButton.arc`, `HomeButtonSe.arc`, `homeBtn_ENG.szs`,
`SpeakerSe.arc`, `home.csv`, `config.txt` and `homeBtnIcon.tpl`.

## Pinned attribution

Pinned WiiCompiled:
`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision:

```cpp
uint32_t word0 = Memory::Read32(oa + 0x00);
word0 = (word0 & ~0xFu) | (ws & 0x3u) | ((wt & 0x3u) << 2);
Memory::Write32(oa + 0x00, word0);
GXInitTexObjWrapMode(obj, (GXTexWrapMode)ws, (GXTexWrapMode)wt);
```

For the hardware tuple `ws=0`, `wt=0`, and the proven post-LOD
`word0=0x00000195`, the guest result is:

```text
word0: 0x00000195 -> 0x00000190
word1: 0x00000000 -> 0x00000000
```

## Switch candidate

Accept only:

```text
obj   = 0x9018E120
wrapS = 0 (GX_CLAMP)
wrapT = 0 (GX_CLAMP)
```

and require the exact already-proven post-LOD guest descriptor before the
mutation.

The rendered path must reuse the existing host GXTexObj and call
`GXInitTexObjWrapMode(..., GX_CLAMP, GX_CLAMP)`.

Do not pre-port `GXInitTexObjTlut`, `GXInitTexObjFilter`,
`GXInitTexObjLODBias` or neighboring texture APIs.

## Hardware acceptance

A later real-Switch run must move the durable frontier beyond
`GXInitTexObjWrapMode (0x80170B50)` and preserve resource, scheduler, FIFO,
GXCopyDisp and present invariants.
