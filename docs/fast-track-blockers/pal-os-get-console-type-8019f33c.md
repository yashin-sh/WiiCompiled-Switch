# Fast-track blocker: PAL OSGetConsoleType (0x8019F33C)

Hardware record after the OSReport fast-track HLE:

- dispatch kind: `DIRECT`
- target: `0x8019F33C`
- guest PC snapshot: `0x800060A4`
- fast-track stage: `TRANSLATED_EXEC_ENTER`

Pinned WiiCompiled (`a135beb201042b20f390c6695ca6b26768820fb4`) registers this PAL entry point as a native `OSGetConsoleType` override. It reads the guest physical MEM2 size from `0x80003118` and returns `0x00000012` for a 64 MiB retail MEM2, otherwise `0x10000012` to expose the NDEV/expanded-memory path.

The Switch fast-track therefore mirrors that guest-visible result instead of treating this boundary as a no-op.
