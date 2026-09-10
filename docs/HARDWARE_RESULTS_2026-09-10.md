# Hardware results — 2026-09-10

Real Nintendo Switch hardware validation of the Horizon runtime bootstrap and translated-product boundary.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

## Runtime bootstrap — PR #17

The first returned `runtime-bootstrap.txt` for `main` commit `ba72ba8b07979ca9993f02cc19a25209c0a06648` reported:

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
- historical stop point: `WAITING_FOR_USER_DATA`.

The reported `game-data` directory was not present. This did not block the core bootstrap and was subsequently replaced by the correct build-time translated-product model in PR #20.

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

## Translated-product boundary — PR #20

PR #20 was merged to `main` as `8446eb8f16aa9b2f6d7486a2f25d3a8c29bc5ada`. The public `0.0.3` NRO adds a Nintendo-data-free weak translated-product seam and separates runtime SD data from build-time translated code.

A fresh real-Switch run returned:

- critical SDL path: `NONE`;
- lifecycle: `READY`;
- filesystem: `READY`;
- timing: `READY`;
- libnx HID: `READY`;
- `Memory::Init`: `READY`;
- HostContext scheduler: `READY`;
- HostContext handoff #1: `READY`;
- HostContext continuation: `READY`;
- audio backend: `STUBBED`;
- graphics backend: `STUBBED`;
- translated product: `NOT LINKED`;
- translated product ABI: `expected=1 reported=0`;
- translated product id: `<none>`;
- translated build: `Nintendo-data-free stub`;
- runtime data root: `sdmc:/switch/WiiCompiled-Switch`;
- logs root: `sdmc:/switch/WiiCompiled-Switch/Logs`;
- cache root: `sdmc:/switch/WiiCompiled-Switch/Cache`;
- config root: `sdmc:/switch/WiiCompiled-Switch/Config`;
- NAND root: `sdmc:/switch/WiiCompiled-Switch/NAND`;
- stop point: `WAITING_FOR_TRANSLATED_PRODUCT`.

This exactly matches the intended Nintendo-data-free boundary. No translated game code was linked or executed.

## Decision

The M2 Horizon runtime bootstrap and translated-product boundary are hardware-validated. The following foundations can now be treated as proven on real Switch hardware:

1. dynamic 4 GiB guest-address reservation strategy;
2. checked heap-backed GuestFlat model;
3. Wii `Memory::Init` integration;
4. AArch64 `HostContext` implementation through the upstream public API;
5. libnx lifecycle, filesystem root, monotonic timing/sleep and HID bootstrap services;
6. SDL-free critical bootstrap path;
7. explicit runtime SD roots for logs/cache/config/NAND;
8. weak/strong build-time translated-product seam;
9. clean `WAITING_FOR_TRANSLATED_PRODUCT` stop when no local product is linked.

Audio and graphics remain intentionally outside this validation.

## Next boundary

At the pinned upstream revision, `SystemBridge::Initialize()` initializes Wii memory, seeds low memory, calls the translator-generated `InitializeDataSections()`, initializes the persistent PowerPC CPU context, then begins executing translated static constructors before the main game path.

The next workstream should therefore remain local-only and staged:

1. generate the Mario Kart Wii translated product from a user-owned DOL/REL;
2. link a strong product adapter and verify `TRANSLATED_PRODUCT_LINKED` without executing game code;
3. wire generated `InitializeDataSections()` behind an explicit execution-handoff API;
4. attempt the earliest translated constructor/entry-point handoff and stop on the first real missing runtime/HLE dependency.

Game-derived inputs, generated translated output and game-containing NRO artifacts remain excluded from this repository and public CI.
