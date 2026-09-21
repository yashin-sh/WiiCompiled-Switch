# Hardware result — first RMCP01 FIFO render work / AsyncDisplay endRender frontier (2026-09-21)

Tracking: #117, #162

## Result

The latest rendered real-Switch RMCP01 run hardware-crosses PAL
`GXBegin (0x8016F0F0)` and, for the first time, produces real game-facing
drawable work through the pinned FIFO/Aurora path.

The rendered graphics report records:

```text
FIFO EVENT #11 size=4 value=0x0f000000 float=0
FIFO EVENT #12 size=4 value=0x00000000 float=1
PASS FIRST_RMCP01_FIFO_WORK
FIFO EVENT #13 size=4 value=0x00000000 float=1
FIFO EVENT #14 size=4 value=0x00000000 float=1
FIFO EVENT #15 size=4 value=0x44200000 float=1
FIFO EVENT #16 size=4 value=0x00000000 float=1
```

This is the first accepted proof that real RMCP01 vertex payload has crossed
the `GXBegin` boundary and caused the pinned `HleFifoWrite` path to mark
Aurora frame work.

The new durable blocker is:

```text
kind   : INDIRECT_CALL_MISS
target : 0x8020ff9c
r1     : 0x803990a8
r2     : 0x8038efa0
r3     : 0x8042944c
r4     : 0x00000000
r5     : 0x00000004
r13    : 0x8038cc00
stage  : RMCP01_FIFO_RENDER_WORK
action : abort after durable blocker record
```

The stage is written only after real FIFO work is observed, so the blocker is
durable proof of progression beyond `GXBegin`.

## Snapshot ordering

An earlier durable post-main snapshot at 2,393 translated dispatches still
shows `GXBegin hits = 0`, nine FIFO writes, and
`FIFO produced work = NO`. That snapshot predates the final transition and
does not contradict the later rendered graphics report or blocker record.

The independent watchdog remains ACTIVE and the FST remains structurally
published at `0x97DC0000`, 64,224 bytes / 2,096 entries.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x8020FF9C` is the native override:

```cpp
extern "C" void EGG__AsyncDisplay__endRender_HLE_8020ff9c(CpuContext* ctx) {
    uint32_t p = ctx->gpr[3];
    ctx->gpr[3] = p;
    ctx->lr = 0x8020FF9C;
    InvokeIndirectCpu(0x80219FB4u, ctx);
    InvokeIndirectCpu(0x8016ED50u, ctx);
}
```

The two guest callees are:

```text
0x80219FB4 = EGG::Display::copyEFBtoXFB
0x8016ED50 = GXSetDrawDoneCallback
```

## Narrow Switch implementation

The candidate resolves only the hardware-observed indirect native override
`0x8020FF9C`.

It mirrors the pinned contract:

- preserve incoming `r3`;
- set guest LR to `0x8020FF9C`;
- dispatch `0x80219FB4` through the existing translated indirect table;
- then dispatch `0x8016ED50` through the same table.

No direct `GXCopyDisp`, present, callback result, EFB copy success, or later
render boundary is fabricated. If either nested target is absent from the
current private translated product, the normal dispatcher will record that
exact next hardware blocker.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `AsyncDisplay endRender` hit count must become non-zero;
2. `0x8020FF9C` must no longer be the durable blocker;
3. preserve `FIFO produced work = YES`;
4. preserve scheduler/FST/renderer invariants;
5. inspect whether `EGG::Display::copyEFBtoXFB` reaches the existing
   `GXCopyDisp` bridge;
6. record any first `GXCopyDisp > 0` and present result;
7. if a nested indirect target is missing, that exact target becomes the next
   frontier.

The following boundary must come from hardware.
