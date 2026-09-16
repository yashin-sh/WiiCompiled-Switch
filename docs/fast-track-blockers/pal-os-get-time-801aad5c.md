# PAL OSGetTime — 0x801AAD5C

Real Switch hardware exposed `0x801AAD5C` as the next direct blocker after the merged `PADInit` bridge was crossed.

Pinned WiiCompiled maps this address to `OS__GetTime_HLE`. It performs a rollover-safe Broadway time-base read (`TBU`, `TBL`, `TBU`, retry if upper changed) and returns the stable high/low words in guest `r3:r4`.

No guest-memory, scheduler, callback, or device state is touched. The Switch bridge therefore reuses the existing `PPC_Mftbu()` / `PPC_Mftb()` helpers and does not pre-port adjacent time APIs.
