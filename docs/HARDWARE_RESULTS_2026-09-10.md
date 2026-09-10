# Hardware results — 2026-09-10

Real Nintendo Switch hardware validation of the Horizon runtime bootstrap merged in PR #17 (`main` commit `ba72ba8b07979ca9993f02cc19a25209c0a06648`).

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Runtime bootstrap

The returned `runtime-bootstrap.txt` reports:

- critical SDL path: `NONE`;
- lifecycle: `READY`;
- filesystem: `READY`;
- timing: `READY`;
- libnx HID: `READY`;
- `Memory::Init`: `READY`;
- upstream `HostContext` scheduler: `READY`;
- first HostContext handoff: `READY`;
- HostContext continuation: `READY`;
- audio backend: `STUBBED` (intentional);
- graphics backend: `STUBBED` (intentional);
- stop point: `WAITING_FOR_USER_DATA`.

The reported `game-data` directory was not present. This did not block the core bootstrap and is not interpreted as a game-loader failure.

## VM and memory regression results

The accompanying `vm-probe.txt` remained fully green:

- full 4 GiB guest VA candidate found and reservation succeeded;
- `svcMapMemory` heap -> stack test succeeded, including writeback;
- SharedMemory dual mapping succeeded in both directions;
- SharedMemory guest/host queries succeeded;
- AArch64 cooperative context probe: `100000 / 100000` switches, PASS;
- callee-saved register preservation: PASS;
- checked heap-backed GuestFlat smoke: PASS;
- upstream-facing GuestFlat API smoke: PASS;
- `Memory::Init` smoke: PASS;
- 7/7 memory regions described;
- MEM1/MEM2 alias identity and big-endian scalar access: PASS;
- locked cache: PASS;
- reset/teardown: PASS.

## Decision

The M2 Horizon runtime bootstrap is hardware-validated. The following foundations can now be treated as proven on real Switch hardware:

1. dynamic 4 GiB guest-address reservation strategy;
2. checked heap-backed GuestFlat model;
3. Wii `Memory::Init` integration;
4. AArch64 `HostContext` implementation through the upstream public API;
5. libnx lifecycle, filesystem root, monotonic timing/sleep and HID bootstrap services;
6. SDL-free critical bootstrap path.

Audio and graphics remain intentionally outside this validation.

## Architecture correction after validation

The `WAITING_FOR_USER_DATA` label is a temporary bootstrap boundary, not the intended final WiiCompiled loading architecture. At the pinned upstream revision, the user's DOL/REL inputs are translated at build time into generated C++ plus `RuntimeConfig.h` / data initialization, then linked together with the runtime into one native executable.

Therefore future work must distinguish:

- user-owned build-time source inputs used locally by the translator;
- generated translated product linked into the NRO;
- runtime SD-card data such as configuration, NAND/save-compatible state, logs/cache and later runtime/mod assets.

The next boundary work is tracked in issue #18. Game-derived inputs and generated output remain excluded from this repository and CI.
