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

The original Switch bridge mirrored only the pin's non-fiber path. That was
sufficient for the two early `EGG::Video::configure` waits, before any guest
OSThread fiber owns the translated host stack.

The 2026-09-19 rendered hardware run exposes why that is insufficient later:
OSThread `0x90112660` runs at priority 6 while the default thread remains
READY at priority 16. The same run records 928 `VIWaitForRetrace` calls but
only 6 `SelectThread` calls. Because the old bridge used `svcSleepThread`
without parking the guest OSThread, the priority-6 guest remained RUNNING and
starved the default thread.

The Switch runtime now has HostContext-backed guest fibers, so the bridge
matches both pinned paths:

- before a guest fiber is registered, preserve the paced non-fiber path used by
  the early video bootstrap;
- once a guest fiber is active, disable guest interrupts, park the current
  OSThread on VI queue `0x80386BC0` through `OSSleepThread`, and resume only
  after the retrace count changes;
- poll already-due retraces synchronously at translated/native call boundaries,
  with a re-entry guard and bounded catch-up, instead of mutating guest RAM from
  a concurrent Horizon thread;
- each retrace publishes the new count, wakes the VI queue, then preserves the
  pinned pre/post callback ordering.

This changes scheduling semantics only where the hardware evidence proves the
non-fiber approximation was wrong. Renderer and DVD behavior are unchanged.

## Hardware validation

The bridge was hardware-crossed on 2026-09-15. The subsequent run reached `VISetPostRetraceCallback` (`0x801B9138`), and after PR #137 crossed that registration boundary the next durable blocker became `OSCreateThread` (`0x801A9E84`).

This confirms `0x801B99EC` is no longer an unsupported direct dispatch in the observed post-main path.

## 2026-09-19 later-thread evidence

Guest-thread lifecycle telemetry identifies the durable worker as:

```text
OSThread : 0x90112660
entry    : 0x8024373C  EGG::Thread::start(void*)
arg      : 0x8042E930
vtable   : 0x80270BC0
vt_run   : 0x80008D18
priority : 6
```

This is distinct from the initial `EGG::ProcessMeter` thread. The exact class
name for `0x80008D18` is not yet established in the public decomp, but that is
no longer required to explain the starvation: the pinned fiber-aware
`VIWaitForRetrace` contract directly matches the observed counters and guest
scheduler state.
