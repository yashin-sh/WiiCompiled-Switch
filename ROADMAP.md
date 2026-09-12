# Roadmap

## M0 — libnx bootstrap
- [x] Minimal AArch64 `.nro` target
- [x] Atmosphère/hbmenu launch loop
- [x] Joy-Con / controller input smoke test
- [x] SD-card filesystem smoke test
- [x] CI build with devkitA64

## M1 — WiiCompiled platform audit
- [x] Pin a known-good WiiCompiled commit
- [x] Inventory OS/platform abstractions used by Windows/Linux/macOS
- [x] Identify POSIX assumptions incompatible with Horizon/libnx
- [x] Produce portability/blocker matrix for AArch64 Switch target

## M2 — runtime bring-up and translated boot fast-track
- [x] HostContext / coroutine backend
- [x] monotonic clock + sleep/yield
- [x] aligned allocation and GuestFlat guest-memory reservation strategy
- [x] generated data initialization handoff
- [x] link and enter real WiiCompiled-translated code locally
- [x] generic translated indirect-dispatch bridge
- [x] durable host exception and unsupported-dispatch diagnostics
- [x] robust SD diagnostic path creation / fallback
- [x] headless local fast-track that avoids the unrelated PrintConsole/NV framebuffer path
- [x] early cache/timebase/interrupt/exception bootstrap HLE
- [x] EXI/SI early bootstrap and basic transaction HLE
- [x] hardware-cross `OSReport` (`0x801A25D0`)
- [x] hardware-cross `OSGetConsoleType` (`0x8019F33C`)
- [x] hardware-cross `OSGetResetCode` (`0x801A8A50`)
- [x] hardware-cross `DCZeroRange` (`0x801A16E4`), including invalid-range/null-pointer handling
- [x] hardware-cross `IPCCltInit` (`0x80193478`) with translated `IPCInit` (`0x80192F7C`) handoff
- [ ] reach PAL Mario Kart Wii `main` (`0x8000B6B0`)
- [ ] identify and fix the first post-`main` runtime blocker
- [ ] complete thread/mutex/condition-variable semantics required by the game
- [ ] complete filesystem/NAND/DVD abstractions required by boot
- [ ] replace remaining temporary runtime/HLE stubs with verified semantics

Hardware evidence is recorded in:

- `docs/HARDWARE_RESULTS_2026-09-10.md`
- `docs/HARDWARE_RESULTS_2026-09-12.md`

The current hardware-driven method is deliberate: execute the broadest safe translated startup path, stop on the first unsupported native/translated boundary or attributable exception, mirror the pinned WiiCompiled semantics, validate in Nintendo-data-free CI, then repeat on hardware.

## M3 — graphics / first frame
- [ ] Select the native Switch graphics strategy compatible with WiiCompiled/Aurora
- [ ] Replace the temporary GX FIFO sink with a real GX → Switch command/backend path
- [ ] Render first clear frame
- [ ] GX command path functional
- [ ] shader/pipeline cache strategy
- [ ] 720p handheld / 1080p docked policy

> Graphics is intentionally not considered validated while `GX_HLE_FIFO_Write*` remains a sink. A black screen during the M2 fast-track is therefore not proof of a graphics failure.

## M4 — input + audio
- [ ] Map Joy-Con / Pro Controller to WiiCompiled input
- [ ] Rumble
- [ ] Audio output
- [ ] latency instrumentation

## M5 — first game boot / playable offline path
- [x] User-owned local translation/build pipeline exists
- [x] enter translated Mario Kart Wii startup on real Switch hardware
- [ ] reach game `main()`
- [ ] complete game/resource initialization
- [ ] menus
- [ ] offline time trial
- [ ] Grand Prix / VS

## M6 — performance
- [ ] CPU profiling on Horizon
- [ ] remove costly synchronization
- [ ] reduce translated-state overhead
- [ ] graphics submission optimization
- [ ] frame pacing
- [ ] target stable 60 FPS where feasible

## M7 — online/mod compatibility
- [ ] save data
- [ ] RetroWFC compatibility
- [ ] Retro Rewind compatibility evaluation
