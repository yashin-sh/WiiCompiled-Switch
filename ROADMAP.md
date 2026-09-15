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
- [x] hardware-cross `__OSInitSTM` (`0x801AB848`) with guest SDA state and fake STM handles
- [x] hardware-cross `NANDInit` (`0x8019E18C`) with SD-backed NAND root and guest home/init state
- [x] hardware-cross `NANDPrivateOpenAsync` (`0x8019C990`) far enough to reach `SCCheckStatus`; SD-backed open state and guest completion ABI are active
- [x] hardware-cross `SCCheckStatus` (`0x801B0220`) far enough to reach `DVDInit`; pinned immediate `SC_STATUS_OK` semantics are active
- [x] hardware-cross `DVDInit` (`0x8015EA1C`) far enough to reach `DVDLowClearCoverInterrupt`; startup-visible guest bookkeeping is active without fabricating FST data
- [x] hardware-cross `DVDLowClearCoverInterrupt` (`0x80166964`) far enough to reach `DVDLowInquiry`; pinned immediate-success semantics are active
- [x] hardware-cross `DVDLowInquiry` (`0x80165A30`) far enough to reach `ESP_InitLib`; command-block completion and shared DVD cancel/reset bookkeeping are active
- [x] hardware-cross `ESP_InitLib` (`0x801671D0`) far enough to reach `ESP_CloseLib`; pinned immediate-success semantics are active with no host `/dev/es` dependency
- [x] hardware-cross `ESP_CloseLib` (`0x80167224`) far enough to reach `NANDOpenAsync`; pinned no-op close and immediate-success semantics are active
- [x] hardware-cross `NANDOpenAsync` (`0x8019C918`) far enough to reach `NANDReadAsync`; SD-backed open state and guest completion ABI are active
- [x] hardware-cross `NANDReadAsync` (`0x8019B80C`) far enough to reach `NANDCloseAsync`; raw byte-count callback semantics and OK-zero async return are active
- [x] hardware-cross `NANDCloseAsync` (`0x8019CAEC`) far enough to reach PAL `main`; persistent-fd close state, closed guest openFlag and verbatim async return semantics are active
- [x] reach PAL Mario Kart Wii `main` (`0x8000B6B0`) on real Switch hardware after 605 translated dispatches
- [x] prove post-main execution enters `System::RKSystem::main` (`0x80008EF0`) and `System::RKSystem::initialize` (`0x80009194`)
- [x] identify and fix the first post-`main` runtime blocker under #117: missing Wii boot low-memory/MEM2 arena seed before the second `OSInitAlloc` (`0x801A0FC8`)
- [x] hardware-cross the repaired MEM2 allocation path far enough to reach `OSLockMutex` (`0x801A7EE4`)
- [x] hardware-cross `OSLockMutex` far enough to reach `OSGetCurrentThread` (`0x801A98B0`)
- [x] hardware-cross `OSGetCurrentThread` far enough to reach PAL `GXInit` (`0x8016B850`)
- [x] hardware-cross the PAL `GXInit` bridge far enough to reach PAL `VIInit` (`0x801B94A4`)
- [x] hardware-cross the PAL `VIInit` / `__VIInit` bridge far enough to reach `SCGetEuRgb60Mode` (`0x801B1CAC`)
- [x] hardware-cross the pinned PAL60 `SCGetEuRgb60Mode` bridge far enough to reach `SCGetAspectRatio` (`0x801B1BE4`)
- [x] hardware-cross the pinned-default widescreen `SCGetAspectRatio` bridge far enough to reach `VIGetDTVStatus` (`0x801BAD38`)
- [x] hardware-cross the pinned `VIGetDTVStatus` disabled/not-ready bridge far enough to reach `VISetBlack` (`0x801BAB2C`)
- [x] hardware-cross the pinned `VISetBlack` pending-state bridge far enough to reach `VIConfigure` (`0x801B9F6C`)
- [x] hardware-cross the pinned `VIConfigure` pending-state bridge far enough to reach `VIFlush` (`0x801BA9A4`)
- [x] hardware-cross the pinned `VIFlush` pending-state arm far enough to reach `GXSetDispCopySrc` (`0x8016F438`)
- [x] hardware-cross the pinned `GXSetDispCopySrc` state/FIFO bridge far enough to reach `GXSetDispCopyDst` (`0x8016F4B8`)
- [ ] hardware-validate the pinned `GXSetDispCopyDst` state/FIFO bridge and identify the next post-main blocker
- [ ] publish a real local DVD FST/data mapping before resource loading requires it
- [ ] move NAND async completion draining from the fast-track HLE boundary to a verified alarm/IOS scheduling point if later hardware ordering requires it
- [ ] complete thread/mutex/condition-variable semantics required by the game
- [ ] complete filesystem/NAND/DVD abstractions required by boot
- [ ] replace remaining temporary runtime/HLE stubs with verified semantics

Hardware evidence is recorded in:

- `docs/HARDWARE_RESULTS_2026-09-10.md`
- `docs/HARDWARE_RESULTS_2026-09-12.md`
- `docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`
- `docs/HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md`

The current hardware-driven method remains deliberate after `main`: execute the broadest safe translated path, stop on the first unsupported native/translated boundary or attributable exception, mirror the pinned WiiCompiled semantics, validate in Nintendo-data-free CI, then repeat on hardware. Post-main bring-up remains tracked in #117.

## M3 — graphics / first frame
- [ ] Resolve shared upstream GX safety blockers before attributing failures to a Switch backend:
  - [ ] #109 — release-safe FIFO bounds checking
  - [ ] #110 — prevent draw merges across different `GXVtxFmt` values
  - [ ] #111 — guard `GX_LINESTRIP` zero/short vertex counts
  - [ ] #112 — make unsupported indexed XF loads visible in Release builds
- [ ] Select the native Switch graphics strategy compatible with WiiCompiled/Aurora
- [ ] Replace the temporary GX FIFO sink with a real GX → Switch command/backend path
- [ ] Render first clear frame
- [ ] GX command path functional
- [ ] shader/pipeline cache strategy
- [ ] 720p handheld / 1080p docked policy

The upstream GX audit behind issues #109–#112 is recorded in `docs/UPSTREAM_GX_AUDIT_2026-09-13.md`. These are shared decoder/runtime concerns and must be separated from Switch-backend-specific failures during M3.

> Graphics is intentionally not considered validated while `GX_HLE_FIFO_Write*` remains a sink. A black screen during the fast-track is therefore not proof of a graphics failure.

## M4 — input + audio
- [ ] Map Joy-Con / Pro Controller to WiiCompiled input
- [ ] Rumble
- [ ] Audio output
- [ ] latency instrumentation

## M5 — first game boot / playable offline path
- [x] User-owned local translation/build pipeline exists
- [x] enter translated Mario Kart Wii startup on real Switch hardware
- [x] reach game `main()` on real Switch hardware
- [x] capture and fix the first post-main blocker (#117)
- [ ] continue post-main initialization blocker-by-blocker through system/resource initialization
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
