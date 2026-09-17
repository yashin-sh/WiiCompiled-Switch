# Hardware result: sustained post-main translated liveness

Date: 2026-09-17
Tracking: #117

## Observation

After PR #159 (`OSWakeupThread`) was merged, the next real-Switch run no longer returned automatically to hbmenu through an unsupported-dispatch abort. The user observed a prolonged black screen and terminated the application manually.

The final translated heartbeat was:

```text
dispatch count        : 37148
post-main dispatch    : 36543
last target           : 0x8020fcd4
guest pc              : 0x8024373c
r1                    : 0x90112608
r2                    : 0x8038efa0
r3                    : 0x00000f8f
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_ENTER
PAL main              : 0x8000b6b0
main reached          : YES
```

This is qualitatively different from the earlier blocker-driven runs: translated execution progressed tens of thousands of dispatches after PAL `main()` without reaching a new unsupported direct/indirect boundary.

## Attribution

RMCP01 `doldecomp/mkw` splits place `0x8020FCD4` at the start of the `egg/core/eggAsyncDisplay.cpp` text range (`0x8020FCD4..0x802104EC`). This is evidence that execution has reached the EGG display subsystem, not proof that a real frame is rendered.

The current Switch fast-track still consumes GX FIFO writes through the temporary `GX_HLE_FIFO_Write*` sink. A black screen is therefore expected and cannot by itself distinguish a healthy game/display loop from a translated-thread stall.

## Diagnostic follow-up

A separate Horizon watchdog now samples `fast-track-heartbeat.txt` once per second and writes:

```text
sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat-history.txt
```

Each line records host elapsed time, `ACTIVE` vs `STALE`, consecutive stale seconds, total/interval dispatch counts, post-main count, last target and selected guest registers.

The watchdog is deliberately independent of translated dispatch. If guest translated execution stops entirely while the process remains alive, the watchdog continues producing `STALE` samples. This turns the next prolonged black-screen run into an attributable liveness result without modifying guest CPU, memory or scheduler state.

The history is bounded to 180 one-second samples. Normal active samples are fsynced periodically; stale samples are fsynced immediately.
