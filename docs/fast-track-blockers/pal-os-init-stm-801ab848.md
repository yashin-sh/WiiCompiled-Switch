# PAL `__OSInitSTM` — `0x801AB848`

Real Switch hardware reached this unsupported direct boundary after the `IPCCltInit` HLE had been crossed:

```text
kind                  : DIRECT
target                : 0x801ab848
guest pc              : 0x800060a4
r1                    : 0x80399168
r2                    : 0x8038efa0
r3                    : 0x00000000
r13                   : 0x8038cc00
fast-track stage      : TRANSLATED_EXEC_ENTER
```

Pinned WiiCompiled maps `0x801AB848` to `__OSInitSTM_HLE_801ab848`.

The host HLE deliberately does not open Wii `/dev/stm/*` IOS devices. Instead it publishes the guest-visible state that later reset logic expects in the SDA block relative to `r13`:

- `r13 - 0x62CC` = STM initialized flag `1`;
- `r13 - 0x62C8` = non-zero immediate handle `0x00535401`;
- `r13 - 0x62C4` = non-zero event-hook handle `0x00535402`;
- return value = success (`r3 = 1`).

If the SDA base/range is invalid, the Switch HLE returns failure (`r3 = 0`) instead of performing an invalid host memory access.

The Power/Reset callback pointers remain unset because the Switch fast-track does not generate the Wii STM hardware interrupt that would invoke them.

Public CI coverage remains Nintendo-data-free: the synthetic probe exercises the safe zero-`r13` path and verifies that the native dispatch boundary remains linkable under devkitA64.
