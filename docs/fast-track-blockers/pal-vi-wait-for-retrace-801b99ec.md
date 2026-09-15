# PAL VIWaitForRetrace — 0x801B99EC

Tracking: #117

## Hardware evidence

A real Switch run from `main` after PR #135 crossed PAL `GXSetDispCopyDst` and stopped at:

```text
kind    : DIRECT
target  : 0x801B99EC
pc      : 0x800060A4
r1      : 0x80399108
r2      : 0x8038EFA0
r3      : 0x00000001
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

## Pinned WiiCompiled mapping

At pinned commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801B99EC` is `VIWaitForRetrace()`.

The native HLE has two paths:

- when the desktop `GuestFiberManager` is initialized, disable interrupts, sleep on the VI retrace queue until `retraceCount` changes, then restore interrupts;
- otherwise, compute the next VI deadline from `lastRetrace + retraceInterval`, sleep until that deadline when necessary, call `AdvanceRetrace`, and finally return `r3 = 0`.

`AdvanceRetrace` commits state armed by `VIFlush`, increments/toggles retrace state, publishes the guest retrace count, wakes the VI queue, services callbacks, and may service Aurora on desktop.

## Switch fast-track contract

The Switch fast-track does not link the desktop `GuestFiberManager`, so this bridge deliberately mirrors the pin's non-fiber path:

- pace one retrace using the active VI timing (`20 ms` only for PAL format 1, otherwise `16.666 ms`);
- commit pending VI configuration/black/framebuffer state only when `VIFlush` armed it;
- increment the guest-visible retrace count and preserve the pin's callback ordering;
- hand off to guest `OSWakeupThread` if the VI queue is ever non-empty;
- return `r3 = 0`.

Aurora, framebuffer presentation and a real GX/VI renderer remain intentionally absent. The existing headless FIFO sink is unchanged.

## Acceptance

On the next real-Switch run, `0x801B99EC` must no longer be reported as an unsupported direct dispatch. The next durable blocker defines the following step.
