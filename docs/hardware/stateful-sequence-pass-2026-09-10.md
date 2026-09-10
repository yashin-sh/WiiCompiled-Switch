# Stateful translated sequence hardware validation — 2026-09-10

This report records Nintendo-data-free validation metadata only. No generated game code, game data, ROM, keys, firmware, or local NRO is included.

## Hardware result

A real Nintendo Switch running Atmosphère via hbmenu title override executed the guarded PAL RMCP01 translated sequence introduced by PR #37.

Observed runtime checkpoint:

- generated data sections initialized successfully;
- translated execution handoff enabled and provider linked;
- translated execution ABI expected/reported `1/1`;
- translated execution runner available;
- sequence start guest address `0x80006090` (`func_80006090`, `__set_debug_bba`);
- ABI registers: `r1=0x81700000`, `r2=0x8038efa0`, `r13=0x8038cc00`;
- `r3` changed from sentinel `0xa5a5a5a5` to `0x00000001` after executing the setter then getter sequence;
- runtime returned cleanly to the Horizon bootstrap.

The `r3=1` result proves that two real WiiCompiled-translated functions executed consecutively against shared Wii guest state: the first wrote `__debug_bba`, and the second observed that write.

The original hardware candidate reported the generic `TRANSLATED_FUNCTION_EXECUTED` stop point because PR #37 intentionally reused the already validated execution ABI/reporting boundary. The follow-up reporting change adds the dedicated `TRANSLATED_SEQUENCE_EXECUTED` checkpoint without widening guest-code execution.

Tracking: #36.
