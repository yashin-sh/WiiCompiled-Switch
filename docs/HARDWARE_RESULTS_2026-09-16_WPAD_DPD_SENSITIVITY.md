# Hardware result: PAL WPADGetDpdSensitivity frontier

Date: 2026-09-16
Tracking: #117

## Hardware evidence

After merging #147 (`WPADInit`), the real Switch fast-track no longer stops at `0x801BF5C4`.

The next durable blocker is:

- kind: `DIRECT`
- target: `0x801C329C`
- guest PC: `0x800060A4`
- r1: `0x803990B8`
- r2: `0x8038EFA0`
- r3: `0x803457E0`
- r13: `0x8038CC00`
- stage: `HOST_CONTEXT_SWITCH_RETURNED`

This hardware result validates the pinned PAL `WPADInit` bridge far enough to reach the next native WPAD boundary.

## Pinned WiiCompiled semantics

At `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`, `0x801C329C` is `WPADGetDpdSensitivity_HLE`.

The pinned WPAD stub state initializes `dpdSensitivity` to `3`, and the getter simply returns that shared value as `uint32_t`.

No guest-memory access, controller probing, Bluetooth/Wii Remote construction, callback dispatch, or synchronization is part of this boundary.

## Switch bridge

The Switch trait therefore keeps the WPAD DPD sensitivity in the already-shared WPAD HLE state, initializes it to `3`, and writes that value to guest `r3` for `0x801C329C`.

No later WPAD behavior is pre-implemented. The next hardware blocker remains authoritative.
