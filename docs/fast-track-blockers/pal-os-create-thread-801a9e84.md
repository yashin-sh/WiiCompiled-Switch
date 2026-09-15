# PAL OSCreateThread — 0x801A9E84

Tracking: #117

## Hardware evidence

A real Switch run from `main` after PR #137 crossed the VI post-retrace callback registration boundary and stopped at:

```text
kind    : DIRECT
target  : 0x801A9E84
pc      : 0x800060A4
r1      : 0x803990D8
r2      : 0x8038EFA0
r3      : 0x8042A680
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This proves the preceding `VIWaitForRetrace` and `VISetPostRetraceCallback` bridges were crossed on hardware.

## Pinned WiiCompiled mapping

At pinned commit `a135beb201042b20f390c6695ca6b26768820fb4`, `0x801A9E84` is `OSCreateThread()`.

Pinned WiiCompiled performs two classes of work:

- desktop-only host scheduling support: when `GuestFiberManager` is initialized, associate the new guest `OSThread` with a host fiber;
- guest-visible RVL OS semantics: validate priority, initialize the `OSThread` and embedded `OSContext`, seed stack markers/guard and entry argument, initialize priority/suspend/list state, apply the scheduler slow-path fields when active, link the thread into the global thread list under the OS interrupt-state guard, and return success/failure in `r3`.

The Switch fast-track has no desktop `GuestFiberManager`, so host-fiber creation is intentionally omitted. The guest-visible state is still required because later RVL scheduler/thread calls consume the `OSThread` structure directly.

## Switch fast-track contract

The bridge mirrors the pinned guest-visible contract:

- accept `r3..r9` as thread, entry, argument, stack top, stack size, priority and attributes;
- reject priorities outside `0..31` with `r3 = 0`;
- initialize the embedded context with the RVL `OSInitContext` register/`SRR` defaults, including live guest `r2` and `r13`, then apply the `OSCreateThread` LR/r3 overrides;
- preserve the pinned THP decoder GQR2-GQR5 special case;
- seed thread state, attributes, suspend count, priorities, exit value, queues, stack range and guard;
- mirror the scheduler-initialized slow path and global thread-list linkage;
- use the existing Switch interrupt-state HLE while mutating the global thread list;
- return `r3 = 1` on success and `r3 = 0` on invalid guest memory/failure.

No host thread/fiber is fabricated at creation time. Actual resume/scheduling/context-switch semantics remain separate boundaries and will be implemented only when hardware reaches them.

## Acceptance

On the next real-Switch run, `0x801A9E84` must no longer be reported as an unsupported direct dispatch. The next durable blocker defines the following step.
