# Hardware result: WPADGetStatus is the next post-main boundary

Date: 2026-09-16
Tracking: #117

## Hardware evidence

After merged #148 (`WPADGetDpdSensitivity`, PAL `0x801C329C`), the real Switch fast-track advances to:

```text
kind                  : DIRECT
target                : 0x801bf64c
guest pc              : 0x800060a4
r1                    : 0x803990b8
r2                    : 0x8038efa0
r3                    : 0x803457e0
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_RETURNED
action                : abort after durable blocker record
```

This proves the #148 DPD-sensitivity getter is crossed on hardware and keeps the guest HostContext continuation alive.

## Pinned semantics

At `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`, PAL `0x801BF64C` is `WPADGetStatus_HLE`.

It takes no arguments and returns only the shared `WpadContract::State` library status:

- `0` while the contract is not initialized;
- `3` (`kStatusReady`) after `WPADInit` initializes it.

Because the same hardware path already crossed the pinned `WPADInit` bridge, this observed call must return `3`.

## Switch boundary

The fast-track bridge mirrors only that contract-state getter through the existing shared WPAD initialized flag. It does not add controller probing, Bluetooth/Wii Remote state, Joy-Con mapping, callbacks, sync, rumble, or any other WPAD behavior.
