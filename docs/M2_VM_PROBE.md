# M2 — Horizon virtual-memory probe

This probe contains no Nintendo game data. It tests only Horizon/libnx virtual-memory behavior needed by WiiCompiled.

## What it tests

1. Reads the process ASLR region.
2. Queries WiiCompiled's generic AArch64 fixed guest base (`0x1000000000`).
3. Asks libnx whether any contiguous **4 GiB virtual range** is available without allocating 4 GiB of physical RAM.
4. Allocates a small 64 KiB source buffer and maps it to a second virtual address with `svcMapMemory`.
5. Checks alias visibility/writeback.
6. Tests whether the alias can transition `RW -> R -> RW` with `svcSetMemoryPermission`.

## Run on hardware

Build/download the branch artifact and place:

`WiiCompiled-Switch.nro`

at:

`sdmc:/switch/WiiCompiled-Switch/WiiCompiled-Switch.nro`

Launch through hbmenu/Atmosphère. The result is printed on screen and also written to:

`sdmc:/switch/WiiCompiled-Switch/vm-probe.txt`

Send the contents of `vm-probe.txt` back to the project for analysis.

## Important interpretation

- **random 4 GiB VA = FOUND** is a strong positive signal, but does not by itself prove the complete upstream guest-memory model.
- **alias writeback = OK** proves a small dual-view mapping can work with the tested mechanism.
- Permission transition failures do not automatically kill the port; they mean the fault/protection strategy needs a Switch-specific design.
- No deliberate invalid-memory access is performed, so this probe does not intentionally crash the console to test page faults.
