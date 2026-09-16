# Hardware result — PAL OSGetTime frontier (2026-09-16)

## Hardware blocker

A real Switch fast-track run after #151 no longer stopped at PAL `PADInit` (`0x801AF2F0`). The next durable direct blocker was:

```text
kind                  : DIRECT
target                : 0x801aad5c
guest pc              : 0x800060a4
r1                    : 0x803990f8
r2                    : 0x8038efa0
r3                    : 0x80347498
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_WORKER_READY
action                : abort after durable blocker record
```

This hardware result validates the merged `PADInit` bridge far enough to expose PAL `OSGetTime` (`0x801AAD5C`) as the next unsupported native boundary.

## Exact pinned semantics

At `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`, `runtime/src/hle/os/os_time.cpp` native-overrides `0x801AAD5C` with `OS__GetTime_HLE`.

The HLE reads the 64-bit Broadway time base using the SDK rollover-safe sequence:

1. read TBU;
2. read TBL;
3. read TBU again;
4. retry if the two upper words differ;
5. publish the stable upper word in guest `r3` and lower word in guest `r4`.

There is no guest-memory access, scheduler mutation, callback, or device side effect at this boundary.

## Switch bridge scope

The Switch port already supplies `PPC_Mftb()` / `PPC_Mftbu()` from the monotonic Horizon host clock converted through `TimeBaseContract` to Broadway ticks. The new `KnownNativeCpuCall<0x801AAD5C>` reuses those helpers and mirrors only the pinned TBU/TBL/TBU stability loop.

This change deliberately does not pre-port adjacent time APIs or change the existing `__OSGetSystemTime` bridge.

## Next hardware step

Build the merged fast-track NRO, run it on real Switch hardware, and capture the next durable blocker or attributable exception after `OSGetTime` returns.
