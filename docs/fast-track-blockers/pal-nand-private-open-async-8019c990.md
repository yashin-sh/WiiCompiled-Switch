# PAL `NANDPrivateOpenAsync` — `0x8019C990`

Real Switch hardware reached this `DIRECT` boundary from translated PAL `__start` while the fast-track stage was `TRANSLATED_EXEC_ENTER`, proving that the prior `NANDInit` boundary had been crossed.

Pinned WiiCompiled maps `0x8019C990` to `NANDPrivateOpenAsync_HLE`.

Its guest-visible contract is:

1. run the synchronous `NANDOpen` operation with `path`, `NANDFileInfo`, and mode;
2. queue the completion callback with `(result, commandBlock)`;
3. return the same NAND result to the caller.

The Switch bridge maps guest NAND paths below the Horizon SD-backed `nand_root()`, clamps `.`/`..` at that root, opens modes 1/2/3 with a persistent host fd table, writes the fd plus `NANDFileInfo::openFlag = 1`, and preserves the pinned result codes used by this path.

The current fast-track drains queued completions before the native boundary returns, but executes callbacks on a scratch `CpuContext` so callback register changes cannot corrupt the interrupted translated caller. Moving the drain to the future alarm/IOS pump is tracked as a scheduling refinement; the callback ABI and result semantics are already preserved.

Public synthetic coverage passes null guest pointers and a null callback so no Nintendo/game-derived path or callback address is required by CI.
