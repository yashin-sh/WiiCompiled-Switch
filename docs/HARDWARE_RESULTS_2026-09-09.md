# Hardware results — 2026-09-09

Real Nintendo Switch hardware run of the first combined WiiCompiled-Switch portability probes.

## Guest virtual address space

- Target guest VA size: 4 GiB.
- Upstream AArch64 fixed base: `0x1000000000`.
- Horizon ASLR information query succeeded.
- Fixed-base query succeeded.
- A full 4 GiB range at the upstream fixed base was reported free.
- libnx also found a random contiguous 4 GiB VA candidate at `0x000000160f06c000`.

### First alias attempt

- Source allocation succeeded.
- Destination VA was found.
- `svcMapMemory` returned `0x0000dc01`.
- Decoding the Horizon Result gives kernel description 110: `KernelError_InvalidMemoryRange`.
- The v1 probe used `virtmemFindAslr()` for the destination. libnx's own `svcMapMemory` stack-mirroring path uses `virtmemFindStack()`, so this result does **not** establish that aliasing is unavailable.

## AArch64 HostContext

- Worker stack: OK.
- Context initialization: OK.
- First scheduler/worker handoff: OK.
- Continuation: OK.
- Callee-saved register preservation: OK.
- Stress test: `100000 / 100000` switches.
- Stress result: OK.
- Elapsed system ticks: `229727`.

## Decision

The Switch AArch64 cooperative-context implementation is hardware-validated and can be used as the basis for WiiCompiled `HostContext` integration.

Guest-memory work remains open. VM probe v2 will separately test:

1. `svcMapMemory` using a destination from the Horizon stack region, matching libnx's own pattern.
2. Kernel `SharedMemory` mapped twice, with one view at `0x1000000000`, to evaluate a direct guest/host dual-view design suitable for WiiCompiled.
