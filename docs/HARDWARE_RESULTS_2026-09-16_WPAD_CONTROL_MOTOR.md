# Hardware result — PAL WPADControlMotor frontier (2026-09-16)

## Real Switch blocker

After merging #149 (`WPADGetStatus`), the real Switch fast-track advanced to the next unsupported direct boundary:

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : DIRECT
target                : 0x801c0ec4
guest pc              : 0x800060a4
r1                    : 0x803990b8
r2                    : 0x8038efa0
r3                    : 0x00000003
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_SWITCH_RETURNED
action                : abort after durable blocker record
```

This hardware result proves the #149 `WPADGetStatus` bridge was crossed and that post-main execution continued to PAL `WPADControlMotor` at `0x801C0EC4`.

## Pinned WiiCompiled semantics

At `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`, `WPADControlMotor_HLE(uint32_t chan, uint32_t command)` ignores both parameters and returns `void`. It performs no guest-memory access, callback, controller probing, Bluetooth interaction, rumble backend call, or WPAD state mutation.

The Switch bridge therefore mirrors only that exact no-op boundary and preserves the guest register file.

## Scope

No physical controller mapping, rumble implementation, WPAD probing, synchronization, callbacks, Bluetooth/Wii Remote model, or later WPAD entry point is pre-implemented here.

Tracked under #117.
