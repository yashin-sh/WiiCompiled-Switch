# Hardware result — first RMCP01 GPU present / GXFlush frontier (2026-09-21)

Tracking: #117, #162

## Result

The latest rendered real-Switch RMCP01 run crosses the copy/present path and
records the first successful game-facing GPU present with real RMCP01 work:

```text
PASS FIRST_RMCP01_FIFO_WORK
...
PASS FIRST_RMCP01_GX_PRESENT hadWork=1
```

The rendered fast-track writes `PASS FIRST_RMCP01_GX_PRESENT` only after
`g_surface.Present()` succeeds. `hadWork=1` means the presented Aurora
frame had already received real RMCP01 FIFO render work.

The next durable unsupported dispatch is:

```text
kind   : DIRECT
target : 0x8016e654
r1     : 0x80399078
r2     : 0x8038efa0
r3     : 0x90087ee0
r4     : 0x00000001
r5     : 0x00000001
r6     : 0x80245816
r13    : 0x8038cc00
stage  : RMCP01_GX_PRESENTED
action : abort after durable blocker record
```

The `RMCP01_GX_PRESENTED` stage is written only after the successful
`GXCopyDisp` host seam returns from a successful surface present. Therefore
this final durable blocker proves progression beyond:

- `GXSetCopyFilter (0x8016FA40)`;
- the game-facing `GXCopyDisp (0x8016FC38)` boundary;
- the first successful RMCP01 surface present.

A periodic durable snapshot from earlier in the same run still reports
`GXSetCopyFilter hits = 0`, `GXCopyDisp = 0`, and present = 0. That snapshot
predates the final transition and does not contradict the later durable
`RMCP01_GX_PRESENTED` blocker plus renderer PASS record.

This result proves the GPU/presentation milestone. It does **not** by itself
prove that the displayed pixels are a visually correct Mario Kart Wii image;
that requires visual confirmation or stronger game-content evidence.

## Pinned attribution

Pinned WiiCompiled:
`a135beb201042b20f390c6695ca6b26768820fb4`

At that exact revision PAL `0x8016E654` is:

```cpp
extern "C" void GX__Flush_8016e654() { GXFlush(); }
PPC_NATIVE_OVERRIDE_VOID(8016e654, GX__Flush_8016e654, (), ());
```

There are no PPC arguments.

## Narrow Switch implementation

The candidate mirrors only this exact boundary:

- stage `RMCP01_GX_FLUSH`;
- call Aurora `GXFlush()` in the rendered target;
- no-op side-effect in headless/synthetic targets;
- add `GXFlush hits` telemetry;
- preserve the void/no-argument PPC contract.

No neighboring GX state, copy, texture, display-list, DVD, or resource
boundary is pre-ported.

## Preserved invariants

The run still proves:

- PAL main reached;
- FST published at `0x97DC0000`, 64,224 bytes / 2,096 entries;
- renderer initialized;
- real RMCP01 FIFO work;
- successful first game-facing surface present;
- independent watchdog remained active before the final blocker;
- no host exception was required to explain the stop.

## Next hardware acceptance

After public CI and the private rendered build gate:

1. `GXFlush hits > 0`;
2. the durable blocker moves off `0x8016E654`;
3. preserve real FIFO work;
4. preserve successful RMCP01 presentation;
5. preserve scheduler/FST/renderer invariants;
6. classify the next exact game/resource/GX boundary from hardware only;
7. visually confirm the presented game image when possible.

The following boundary must come from hardware.
