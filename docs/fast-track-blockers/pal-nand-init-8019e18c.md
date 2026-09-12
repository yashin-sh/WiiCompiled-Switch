# PAL NANDInit blocker — 0x8019E18C

Real Switch hardware stopped on a `DIRECT` translated boundary at PAL `0x8019E18C` while the fast-track was in `TRANSLATED_EXEC_ENTER`.

Pinned WiiCompiled maps this address to `NANDInit_HLE`.

Guest-visible/runtime semantics mirrored on Switch:

- derive the current four-character game code from guest memory at `0x80000000`, with PAL `RMCP` fallback;
- create the SD-backed title data directory below the existing Horizon `nand_root()`;
- write `/title/00010004/<gamecode>/data` into guest `NANDHomeDir` at `0x80346D20`;
- write `2` to guest NAND initialized state at `0x80386848`;
- return `NAND_RESULT_OK` in `r3`;
- do not attempt to open the Wii IOS `/dev/fs` device on Horizon.

The implementation preserves the pinned host-side intent while keeping public CI Nintendo-data-free. Later NAND/ISFS entry points can reuse the same SD-backed root as they become hardware blockers.
