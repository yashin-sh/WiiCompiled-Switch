# PAL fast-track blocker — VIFlush (`0x801BA9A4`)

Real Switch evidence after PR #132:

```text
kind: DIRECT
target: 0x801ba9a4
pc: 0x800060a4
r1: 0x80399108
r2: 0x8038efa0
r3: 0x00000000
r13: 0x8038cc00
stage: GUEST_POST_MAIN_ACTIVE
```

RMCP01 and pinned WiiCompiled map `0x801BA9A4` to `VIFlush()`.

At pinned WiiCompiled commit `a135beb201042b20f390c6695ca6b26768820fb4`, `VIFlush` ensures VI state exists, then, only when its pending framebuffer slot is still zero, tries to recover the queued framebuffer from the guest SDK globals at `0x80386BA0` and `0x80350890`. A non-zero recovered value becomes the pending framebuffer. The function then sets the internal `flushArmed` flag and returns `r3 = 0`.

Crucially, the pin does **not** commit pending VI state in `VIFlush`; that commit occurs at the next retrace. It also does not mark an XFB ready here; frame readiness is driven later by `GXCopyDisp`.

The Switch fast-track mirrors only that guest/runtime bookkeeping: ensure VI state, recover a queued framebuffer if present, arm the pending-state flag, and return zero. It does not fabricate a retrace, XFB readiness, Aurora state, framebuffer presentation, or a renderer path.

The Nintendo-data-free synthetic probe resolves the same direct target and seeds the hardware-observed `r3 = 0` input shape.

Acceptance: the next hardware run must pass `0x801BA9A4` and expose the next blocker or named phase.
