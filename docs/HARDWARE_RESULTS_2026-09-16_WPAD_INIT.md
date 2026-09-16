# Hardware result: HostContext continuation crossed to WPADInit

Date: 2026-09-16  
Tracking: #117

## Hardware evidence

After merging #146 (`96423feede671bacc94197f08c2fec47004798a0`), the next durable fast-track blocker was:

```text
kind                  : DIRECT
target                : 0x801bf5c4
guest pc              : 0x800060a4
r1                    : 0x803990b8
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_RETURNED
action                : abort after durable blocker record
```

This is the first hardware result after the preserved guest HostContext continuation work. The previous `INDIRECT_JUMP_MISS` at interior SRR0 `0x80238A78` is no longer the frontier; the host-context switch returned normally and execution advanced to PAL `WPADInit` (`0x801BF5C4`).

## Pinned WiiCompiled semantics

Pinned upstream: `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

At that revision `runtime/src/hle/input/wpad.cpp` implements `WPADInit_HLE` by calling `InitializeWpadLibrary()`. That helper only marks the shared WPAD contract initialized and returns `0`.

`runtime/include/hle/controller_status_contract.h` defines `WpadContract::State::Initialize()` as setting its initialized flag to true. No Bluetooth device, Wii Remote object, synchronization callback, probe result, or controller packet is constructed by this boundary.

## Switch implementation boundary

The Switch fast-track therefore only needs to:

- retain a tiny shared `wpad_initialized` host-side state;
- set it to `true` when PAL `WPADInit` is invoked;
- return success (`r3 = 0`);
- leave all other WPAD entry points unsupported until hardware reaches them.

This keeps the blocker-driven rule intact and does not pre-port input behavior that has not yet been observed on hardware.
