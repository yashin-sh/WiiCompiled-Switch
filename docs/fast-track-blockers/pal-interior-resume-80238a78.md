# PAL interior OSContext resume — 0x80238A78

Tracking: #117
Date: 2026-09-16
Pinned WiiCompiled: `a135beb201042b20f390c6695ca6b26768820fb4`

## Hardware blocker

```text
kind    : INDIRECT_JUMP_MISS
target  : 0x80238A78
pc      : 0x80238A78
r1      : 0x803990F8
r2      : 0x8038EFA0
r3      : 0x00000001
r13     : 0x8038CC00
stage   : GUEST_POST_MAIN_ACTIVE
```

The blocker appears after the hardware-proven `OSSleepThread` wait-queue park selected another guest context. The non-fiber `OSLoadContext` fallback restored that context and attempted to dispatch its saved SRR0.

## Mapping

`0x80238A78` is not a PAL function entry. It is inside `EGG::ProcessMeter::__ct`, whose mapped start is `0x8023883C`; the next mapped function starts at `0x80238A94`.

Therefore this address must not be added as a fake translated function root. It represents a scheduler continuation inside an already-active translated function.

## Correct boundary

Pinned WiiCompiled uses per-OSThread host contexts when available. The main guest thread adopts the scheduler host context, created guest threads receive their own host stack, and `SelectThread` switches those host contexts. This preserves the generated C++ continuation naturally and avoids re-entering at an arbitrary PPC instruction.

The Switch fix should use the existing AArch64 `HostContext` backend for that proven path and keep `OSLoadContext` as the fallback for guest contexts without a host continuation.

No `OSWakeupThread`, timer/alarm behavior, or arbitrary interior dispatch shim is part of this blocker fix.
