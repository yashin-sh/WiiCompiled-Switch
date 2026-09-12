# PAL DCZeroRange — 0x801A16E4

Hardware fast-track blocker observed on 2026-09-12 after the headless platform-init fix.

Observed dispatch:

```text
kind                  : DIRECT
target                : 0x801a16e4
guest pc              : 0x800060a4
r1                    : 0x80399158
r2                    : 0x8038efa0
r3                    : 0x8039b180
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

RMCP01 / pinned WiiCompiled mapping: `DCZeroRange`.

Pinned runtime semantics at `a135beb201042b20f390c6695ca6b26768820fb4`:

1. read guest address from `r3` and length from `r4`;
2. return immediately for zero length;
3. align the start down to a 32-byte cache line;
4. round the covered length up to a 32-byte cache-line multiple;
5. zero the aligned guest range;
6. notify the GX host backend that guest RAM was modified by a DMA-style operation;
7. on an invalid guest range, log the memory fault and return without aborting guest startup.

Switch implementation mirrors the guest-visible behavior using `Memory::GetPointer` + `memset`. The GX notification is retained as a Switch-side semantic seam; it is currently a no-op because the M2 GX backend is still a FIFO sink. A future renderer can attach dirty-range tracking to that seam without changing `DCZeroRange` again.

No Nintendo game data or generated translated output is committed by this fix.
