# PAL OSLoadContext — 0x801A1F58

Tracking: #117
Date: 2026-09-16
Pinned WiiCompiled: `a135beb201042b20f390c6695ca6b26768820fb4`

## Hardware blocker

```text
kind    : DIRECT
target  : 0x801A1F58
pc      : 0x800060A4
r1      : 0x803990F8
r2      : 0x8038EFA0
r3      : 0x8042A680
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

This is the first unsupported boundary after the hardware-proven PAL `SelectThread` bridge selected guest context `0x8042A680`.

## Pinned semantics

WiiCompiled native-overrides `OSLoadContext` because the SDK implementation ends in privileged `rfi`. The pinned HLE validates saved SRR0, restores GPR0..31, CR/LR/CTR/XER, GQR1..7 with GQR0 forced to zero, clears OSContext mode bit `0x0002`, restores SRR0/SRR1/PC, and dynamically jumps to SRR0.

The call is not a normal ABI-returning boundary. If the restored target returns to the old scheduler host stack, the pin treats that as invalid and aborts.

## Switch implementation boundary

The Switch bridge mirrors only that proven non-fiber path. It does not create Horizon fibers or invent scheduler state. Restored SRR0 is dispatched through the existing generated indirect table, so a missing translated target remains an exact durable `INDIRECT_JUMP_MISS` blocker.

Nintendo-data-free CI already retains the real SelectThread bridge through `source/main.cpp`; because SelectThread directly depends on `OSLoadContext`, the devkitA64 synthetic fast-track link now covers this helper transitively.
