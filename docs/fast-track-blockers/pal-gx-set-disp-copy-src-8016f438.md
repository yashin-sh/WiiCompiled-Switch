# PAL fast-track blocker — GXSetDispCopySrc (`0x8016F438`)

Real Switch evidence after PR #133:

```text
kind: DIRECT
target: 0x8016f438
pc: 0x800060a4
r1: 0x80399108
r2: 0x8038efa0
r3: 0x00000000
r13: 0x8038cc00
stage: GUEST_POST_MAIN_ACTIVE
```

RMCP01 and pinned WiiCompiled map `0x8016F438` to `GXSetDispCopySrc()`.

At pinned commit `a135beb201042b20f390c6695ca6b26768820fb4`, the HLE narrows `r3..r6` to `u16`, records the display-copy source rectangle in host GX state, then emits two BP/RAS commands through the GX FIFO:

- register `0x49`: top/left origin;
- register `0x4A`: width/height minus one.

The override is `void`, so it does not normalize `r3` or otherwise change guest GPRs. It has no direct guest-memory effect.

The Switch fast-track bridge preserves the rectangle locally and emits the same two FIFO command shapes through the existing `GX_HLE_FIFO_Write*` seam. Those writes remain intentionally consumed by the headless FIFO sink; this blocker does not introduce Aurora, a framebuffer, presentation, or a renderer backend.

The synthetic probe uses the hardware-observed `r3 = 0` and representative non-proprietary values for the remaining arguments to exercise the full four-argument encoding path.

Acceptance: the next hardware run must pass `0x8016F438` and expose the next blocker or named phase.
