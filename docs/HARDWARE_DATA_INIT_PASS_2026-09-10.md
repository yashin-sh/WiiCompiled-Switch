# Hardware PASS — synthetic translated data init — 2026-09-10

Real Nintendo Switch hardware validation for the guarded translated data-section initialization boundary introduced by PR #24.

Upstream WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.

Validated `main` commit: `b2ef46636eb6ac091f049fb9351ce38edcf3a317`.

The Nintendo-data-free `WiiCompiled-Switch-synthetic-data-init.nro` returned:

- critical SDL path: `NONE`;
- lifecycle/filesystem/timing/libnx HID: `READY`;
- `Memory::Init`: `READY`;
- HostContext scheduler, first handoff and continuation: `READY`;
- audio and graphics: `STUBBED` as intended;
- translated product: `LINKED`;
- translated product ABI: `expected=1 reported=1`;
- product id: `synthetic-data-init-product`;
- data-init handoff: `ENABLED`;
- data-init provider: `LINKED`;
- data-init handoff ABI: `expected=1 reported=1`;
- data initializer: `AVAILABLE`;
- data sections init: `PASS`;
- stop point: `DATA_SECTIONS_INITIALIZED`.

The synthetic initializer copied and verified its Nintendo-data-free payload through the same heap-backed guest-memory path that the generated WiiCompiled initializer uses. This proves the guarded callback boundary on real Switch hardware.

No Mario Kart Wii translated functions, constructors, Nintendo code or game assets were included or executed by this probe.

## Next local-only checkpoint

Use a clean user-owned PAL RMCP01 `main.dol` and `StaticR.rel` on Linux, run `scripts/prepare-local-data-init.sh`, build with `MKW_LOCAL_PRODUCT=1`, and validate the real generated `InitializeDataSections()` on hardware.

Only the resulting `runtime-bootstrap.txt` is suitable to share back. The DOL/REL, generated C++/blobs and game-containing NRO must remain local and must never be committed or uploaded to public CI.
