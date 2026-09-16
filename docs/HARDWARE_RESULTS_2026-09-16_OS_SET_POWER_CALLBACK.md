# Hardware result — PAL OSSetPowerCallback frontier (2026-09-16)

## Hardware blocker

A real Nintendo Switch fast-track run produced:

```text
WiiCompiled-Switch unsupported translated dispatch
=================================================
kind                  : DIRECT
target                : 0x801ab75c
guest pc              : 0x800060a4
r1                    : 0x803990f8
r2                    : 0x8038efa0
r3                    : 0x8000b1f4
r13                   : 0x8038cc00
fast-track stage      : HOST_CONTEXT_WORKER_READY
action                : abort after durable blocker record
```

This proves the merged PAL `OSGetTime` (`0x801AAD5C`) bridge was crossed on real hardware far enough to expose PAL `OSSetPowerCallback` (`0x801AB75C`) as the next unsupported native boundary.

## Pinned WiiCompiled semantics

At `patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`, `OSSetPowerCallback`:

- takes the new callback from guest `r3`;
- uses guest `r13` to address the SDA callback slot at `r13 - 0x62B8` and handler-active flag at `r13 - 0x62C0`;
- treats `0x801ABC0C` as the SDK default callback;
- disables interrupts before mutating the callback state and restores the previous interrupt level afterwards;
- returns the previous callback, except that the default callback is exposed to the caller as `NULL` / `0`;
- writes the new callback, or the default callback when the caller passes `NULL`;
- marks the STM event handler active (`1`) if it was inactive;
- does not perform real Horizon/Wii `/dev/stm` IOS registration in the host HLE.

For the observed hardware call, `r3 = 0x8000B1F4` and `r13 = 0x8038CC00`.

## Switch bridge

The Switch bridge mirrors only those guest-visible semantics. It reuses the already validated shared interrupt-state helpers and guest SDA memory. Because the current Switch `Memory::Read/Write*` path aborts on unmapped ranges rather than throwing, the bridge preflights each guest range with `Memory::Contains` while preserving the pin's partial-side-effect ordering.

No real STM/IOS asynchronous event source is added. Hardware has only proven the callback registration bookkeeping boundary, not delivery of a power-button event.

## Next hardware step

Build the merged `main`, run the local fast-track NRO again, and capture the first blocker or attributable exception after `OSSetPowerCallback`.
