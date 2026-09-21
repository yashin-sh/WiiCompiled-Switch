# Hardware result — GXBegin frontier (2026-09-21)

Tracking: #117, #162

## Result

The latest rendered real-Switch RMCP01 run progresses beyond
PAL `GXSetCullMode (0x8016F3B8)` and exposes the first exact draw-primitive
boundary as the next unsupported DIRECT dispatch:

```text
kind   : DIRECT
target : 0x8016f0f0
r1     : 0x80399008
r2     : 0x8038efa0
r3     : 0x00000080
r4     : 0x00000000
r5     : 0x00000004
r13    : 0x8038cc00
stage  : RMCP01_GX_SET_CULL_MODE
action : abort after durable blocker record
```

The stage is set inside the merged GXSetCullMode bridge, so reaching the
distinct later target proves durable progression beyond `0x8016F3B8`.

This is qualitatively different from the preceding state-setting blockers:
the hardware has now reached the guest's first observed `GXBegin` call.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x8016F0F0` is:

```cpp
extern "C" void GX__Begin_8016f0f0(uint32_t t, uint32_t vf, uint32_t nv)
```

The native-call table consumes:

```text
r3 -> primitive
r4 -> vertex format
r5 -> vertex count
```

Hardware captured:

```text
primitive    = 0x80
vertex format= 0
vertex count = 4
```

At the pin, the immediate-mode path republishes the tracked Aurora VCD/VAT
state, initializes `g_hleGxState` for the begin packet, resets the FIFO vertex
assembly state, and lets subsequent real FIFO writes submit the vertices and
open/close the Aurora draw. If display-list recording is active, it writes the
begin command into the list instead.

## Narrow Switch implementation

The candidate mirrors only this exact boundary:

- preserve the pinned display-list recording branch;
- publish VCD/VAT with the pinned immediate-mode options;
- set `currentVtxFmt`, `currentPrim`, `vertsRemaining`, `inBegin`,
  `auroraBeginCalled`, and reset FIFO/vertex assembly state;
- leave actual vertex submission to the already hardware-proven pinned
  `HleFifoWrite` decoder;
- do not fabricate any vertex bytes or force an Aurora draw.

The pinned desktop deferred-timing shim is intentionally not duplicated inside
this native boundary. Switch timing/thread progression remains owned by the
already hardware-proven Switch scheduler/VI path, avoiding nested desktop
fiber/timer behavior.

No following `GXEnd`, texture, display-list, copy, present, DVD, or resource
boundary is pre-ported.

## Preserved invariants

- PAL main remains reached.
- FST remains published at `0x97DC0000`, 64,224 bytes / 2,096 entries.
- renderer remains initialized with an active frame.
- independent watchdog remains ACTIVE.
- rendered graphics log still reaches eleven FIFO state writes.
- the run that exposed `GXBegin` did not yet execute this bridge, therefore
  no claim of drawable FIFO work, `GXCopyDisp`, or present is made.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXBegin hits` must become non-zero;
2. `0x8016F0F0` must no longer be the durable blocker;
3. execution must reach a later durable dispatch or milestone;
4. preserve scheduler/FST/renderer invariants;
5. inspect whether real post-`GXBegin` vertex payload makes
   `FIFO produced work = YES`;
6. record the first `GXCopyDisp` / present if reached;
7. if the blocker remains `GXBegin` or a native exception occurs inside the
   begin/FIFO path, debug only this boundary.

The following boundary must come from hardware.
