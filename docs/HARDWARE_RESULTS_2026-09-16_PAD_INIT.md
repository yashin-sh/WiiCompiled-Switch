# Hardware result — PAL PADInit frontier (2026-09-16)

## Hardware blocker

Real Switch hardware on `main` after PR #150 crossed the pinned PAL `WPADControlMotor` no-op and stopped at:

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : DIRECT
target                : 0x801af2f0
guest pc              : 0x800060a4
r1                    : 0x80399148
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_RETURNED
action                : abort after durable blocker record
```

This hardware result validates the previous `WPADControlMotor` bridge and advances the post-main frontier to PAL `PADInit` (`0x801AF2F0`).

## Pinned WiiCompiled semantics

Pinned upstream: `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`.

`runtime/src/hle/input/pad.cpp` maps `0x801AF2F0` to `PAD__Init_HLE()`, which returns `PADInit() ? 1u : 0u`.

At the same pin, Aurora `PADInit()` is idempotent: if already initialized it returns true; otherwise it marks the host PAD layer initialized, seeds desktop keyboard mappings from defaults, then returns true.

## Switch implementation boundary

The Switch bridge mirrors only the hardware-proven contract needed at this boundary:

- preserve a host-side `g_pad_initialized` state;
- mark it initialized on `PADInit`;
- return guest-visible success (`r3 = 1`);
- do not construct SDL keyboard/gamepad objects on Horizon;
- do not pre-port `PADRead`, `PADReset`, `PADRecalibrate`, `PADControlMotor`, or controller mappings.

Nintendo-data-free compile/link coverage is extended through the synthetic input probe.
