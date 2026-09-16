# PAL OSSetPowerCallback — 0x801AB75C

Real Switch hardware exposed `0x801AB75C` as the next direct blocker after the merged `OSGetTime` bridge was crossed.

Pinned WiiCompiled maps this address to `OSSetPowerCallback_801ab75c`. The native HLE keeps only the SDK-visible callback bookkeeping while stubbing the real STM/IOS registration:

- input callback: guest `r3`;
- SDA callback slot: `r13 - 0x62B8`;
- SDA handler-active flag: `r13 - 0x62C0`;
- SDK default callback: `0x801ABC0C`;
- callback mutation is wrapped by disable/restore-interrupt semantics;
- passing `0` restores the default callback;
- the previous default callback is returned as `0`, otherwise the previous callback address is returned;
- an inactive handler flag is set to `1`.

The Switch bridge intentionally does not implement a real `/dev/stm` event source or power-button delivery. Hardware has only proven that MKW requires this registration/bookkeeping boundary at this point in post-main initialization.

Observed call:

```text
kind             : DIRECT
target           : 0x801ab75c
r3               : 0x8000b1f4
r13              : 0x8038cc00
fast-track stage : HOST_CONTEXT_WORKER_READY
```
