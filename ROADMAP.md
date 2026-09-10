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

## M2 — runtime bring-up
- [x] HostContext / coroutine backend
- [x] monotonic clock + sleep/yield
- [ ] threads, mutexes, condition variables
- [x] aligned allocation and guest-memory reservation strategy
- [ ] filesystem/NAND abstraction on SD card
- [ ] logging and fatal diagnostics

Hardware validation of the current M2 bootstrap was completed on 2026-09-10. See `docs/HARDWARE_RESULTS_2026-09-10.md`.

## M3 — graphics
- [ ] Select Switch graphics strategy compatible with WiiCompiled/Aurora
- [ ] Render first clear frame
- [ ] GX command path functional
- [ ] shader/pipeline cache strategy
- [ ] 720p handheld / 1080p docked policy

## M4 — input + audio
- [ ] Map Joy-Con / Pro Controller to WiiCompiled input
- [ ] Rumble
- [ ] Audio output
- [ ] latency instrumentation

## M5 — first game boot
- [ ] User-owned disc extraction pipeline
- [ ] boot through WiiCompiled translated entry point
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
