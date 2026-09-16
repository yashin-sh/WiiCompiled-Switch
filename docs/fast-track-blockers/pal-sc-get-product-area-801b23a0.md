# PAL SCGetProductArea — 0x801B23A0

Real Switch hardware exposed `0x801B23A0` as the next direct blocker after the merged PAL `OSSetPowerCallback` bridge was crossed.

Pinned WiiCompiled maps this address to `SCGetProductArea_HLE`.

Exact pinned behavior:

- fetch the current emulated NAND `AREA` string;
- scan the SDK product-area table at guest `0x8029CEB0`;
- table stride is `5` bytes and the maximum row count is `13`;
- row byte 0 is the enum value, bytes 1.. are the short NUL-terminated area name;
- row value `0xFF` terminates the table;
- no match returns `0xFFFFFFFF`.

Pinned WiiCompiled's fresh PAL NAND defaults are `AREA=EUR`, `CODE=LEH`, `GAME=EU`. The Switch fast-track currently has no complete console-identity surface, so this boundary uses the same fresh-PAL `EUR` value and resolves it through the local guest table. It does not embed the SDK table in the public repository.

No adjacent SC identity API is implemented by this blocker. In particular, `SCGetProductCode` (`0x801B2424`), `SCGetProductSN` (`0x801B2460`), and `SCGetProductGameRegion` (`0x801B24C8`) remain hardware-gated.
