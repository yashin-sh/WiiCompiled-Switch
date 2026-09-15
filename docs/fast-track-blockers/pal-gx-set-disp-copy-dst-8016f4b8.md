# PAL fast-track blocker — GXSetDispCopyDst (`0x8016F4B8`)

Real Switch evidence after PR #134:

```text
kind: DIRECT
target: 0x8016f4b8
pc: 0x800060a4
r1: 0x80399108
r2: 0x8038efa0
r3: 0x00000260
r13: 0x8038cc00
stage: GUEST_POST_MAIN_ACTIVE
```

RMCP01 and pinned WiiCompiled map `0x8016F4B8` to `GXSetDispCopyDst()`.

Pinned semantics at `a135beb201042b20f390c6695ca6b26768820fb4`:

- narrow `r3`/`r4` to `u16` destination width/height;
- retain both values in host GX display-copy state;
- emit one BP/RAS write through the GX FIFO: `0x4D000000 | ((((width & 0x7FFF) << 1) >> 5) & 0x3FF)`;
- preserve guest GPRs because the native override is `void`;
- no direct guest-memory, VI, framebuffer, presentation, or renderer side effects.

The durable blocker record currently captures `r3` but not `r4`, so only the hardware-observed width (`0x260`) is recorded here. The bridge reads the real runtime `r4`; the synthetic probe uses a representative non-zero height only for Nintendo-data-free compile/link coverage.

The Switch bridge keeps the destination state locally and sends the exact BP command shape to the existing headless FIFO sink. It does not fabricate Aurora or a renderer backend.

Acceptance: the next hardware run must pass `0x8016F4B8` and expose the next blocker or named phase.
