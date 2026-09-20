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
- [x] hardware-cross the pinned `GXSetDispCopyDst` state/FIFO bridge far enough to reach `VIWaitForRetrace` (`0x801B99EC`)
- [x] hardware-cross the pinned non-fiber `VIWaitForRetrace` retrace/commit bridge far enough to reach `VISetPostRetraceCallback` (`0x801B9138`)
- [x] hardware-cross the pinned `VISetPostRetraceCallback` registration bridge far enough to reach `OSCreateThread` (`0x801A9E84`)
- [x] hardware-cross the pinned guest-visible `OSCreateThread` bridge far enough to reach `OS__InitMessageQueue` (`0x801A72FC`)
- [x] hardware-cross the pinned guest-visible `OS__InitMessageQueue` bridge far enough to reach `OSResumeThread` (`0x801AA58C`)
- [x] hardware-cross the pinned `OSResumeThread` guest run-queue/scheduler handoff far enough to reach `SelectThread` (`0x801A9C08`)
- [x] hardware-cross the pinned guest scheduler `SelectThread` bridge far enough to select guest context `0x8042A680` and reach `OSLoadContext` (`0x801A1F58`)
- [x] hardware-cross the pinned non-fiber `OSLoadContext` restore/jump bridge into `EGG::Thread::start` (`0x8024373C`) far enough to reach `OSReceiveMessage` (`0x801A7424`)
- [x] hardware-cross the pinned `OSReceiveMessage` empty blocking-receive path far enough to reach `OSSleepThread` (`0x801AA9B8`) on receive wait queue `0x804294F8`
- [x] hardware-cross the pinned `OSSleepThread` wait-queue park / `SelectThread(0)` handoff far enough to expose saved SRR0 `0x80238A78`, an interior continuation inside `EGG::ProcessMeter::__ct` rather than a translated function entry
- [x] hardware-validate the HostContext-backed guest `OSThread` switch: the original translated host stack resumes past the `0x80238A78` interior continuation and returns from `HostContext::Switch`, exposing PAL `WPADInit` (`0x801BF5C4`) as the next direct blocker
- [x] hardware-cross the pinned PAL `WPADInit` contract initialization far enough to reach `WPADGetDpdSensitivity` (`0x801C329C`)
- [x] hardware-cross the pinned PAL `WPADGetDpdSensitivity` default sensitivity getter far enough to reach `WPADGetStatus` (`0x801BF64C`)
- [x] hardware-cross the pinned PAL `WPADGetStatus` contract-state getter far enough to reach `WPADControlMotor` (`0x801C0EC4`)
- [x] hardware-cross the pinned PAL `WPADControlMotor` no-op boundary far enough to reach `PADInit` (`0x801AF2F0`)
- [x] hardware-cross the pinned PAL `PADInit` idempotent host initialization far enough to reach `OSGetTime` (`0x801AAD5C`)
- [x] hardware-cross the pinned PAL `OSGetTime` rollover-safe time-base getter far enough to reach `OSSetPowerCallback` (`0x801AB75C`)
- [x] hardware-cross the pinned PAL `OSSetPowerCallback` SDA/STM bookkeeping bridge far enough to reach `SCGetProductArea` (`0x801B23A0`)
- [x] hardware-cross the pinned PAL `SCGetProductArea` SDK-table lookup far enough to reach `OSWakeupThread` (`0x801AAAA4`)
- [x] hardware-cross the pinned PAL `OSWakeupThread` wait-queue/run-queue/HostContext handoff into sustained post-main translated execution
- [x] prove a sustained run of 37,148 translated dispatches total / 36,543 post-main without a new unsupported-dispatch abort
- [x] extend sustained execution to 126,563 total / 125,958 post-main translated dispatches
- [x] map `0x8020FCD4` exactly to RMCP01 `PostRetraceCallback` and `0x8024373C` to `EGG::Thread::start(void*)`
- [x] observe callback `r3 = 0x365E`, proving guest VI retrace value advanced to 13,918
- [x] add an independent Horizon liveness watchdog that records ACTIVE vs STALE translated progress without mutating guest state
- [x] classify the sustained black-screen path as an active translated/VI display loop rather than a durable translated-thread stall
- [x] hardware-prove local RMCP01 FST publication at `0x97DC0000` with 64,224 bytes / 2,096 entries (#183)
- [x] disprove `DVDReadPrio` / `DVDReadAsyncPrio` as the current startup frontier: #185 is installed but not reached in the first hardware run
- [x] isolate the durable priority-6 execution to later OSThread `0x90112660`, distinct from the initial `EGG::ProcessMeter` thread, and add lifecycle/vtable telemetry (#186)
- [x] identify its virtual `run()` as `0x80008D18` with object `0x8042E930` / vtable `0x80270BC0`, and correlate the starvation with the stale non-fiber `VIWaitForRetrace` path
- [ ] hardware-validate fiber-aware `VIWaitForRetrace`: priority-6 waiter must park on VI queue `0x80386BC0` and allow the default thread to resume between retraces
- [x] publish the user-owned RMCP01 FST into guest MEM2 and install the narrow local `DATA/files` DVD read mapping (#183/#185); hardware proves FST publication, while the current startup path has not reached the read override yet
- [ ] move NAND async completion draining from the fast-track HLE boundary to a verified alarm/IOS scheduling point if later hardware ordering requires it
- [ ] complete thread/mutex/condition-variable semantics required by the game
- [ ] complete filesystem/NAND/DVD abstractions required by boot
- [ ] replace remaining temporary runtime/HLE stubs with verified semantics

Hardware evidence is recorded in:

- `docs/HARDWARE_RESULTS_2026-09-10.md`
- `docs/HARDWARE_RESULTS_2026-09-12.md`
- `docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`
- `docs/HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md`
- `docs/HARDWARE_RESULTS_2026-09-15_OS_CREATE_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_INIT_MESSAGE_QUEUE.md`
- `docs/HARDWARE_RESULTS_2026-09-16_SELECT_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_LOAD_CONTEXT.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_RECEIVE_MESSAGE.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_SLEEP_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_INIT.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_DPD_SENSITIVITY.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_GET_STATUS.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_CONTROL_MOTOR.md`
- `docs/HARDWARE_RESULTS_2026-09-16_PAD_INIT.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_GET_TIME.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md`
- `docs/HARDWARE_RESULTS_2026-09-16_SC_GET_PRODUCT_AREA.md`
- `docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`
- `docs/HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md`
- `docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md`
- `docs/HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md`
- `docs/HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md`
- `docs/HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md`

The current hardware-driven method remains deliberate after `main`: execute the broadest safe translated path, stop on the first unsupported native/translated boundary or attributable exception, and when no blocker appears use the independent heartbeat watchdog to distinguish sustained execution from a real stall. New runtime behavior is still added only from hardware evidence and pinned WiiCompiled semantics. Post-main bring-up remains tracked in #117.

## M3 — graphics / first frame
- [ ] Resolve shared upstream GX safety blockers before attributing failures to a Switch backend:
  - [ ] #109 — release-safe FIFO bounds checking
  - [ ] #110 — prevent draw merges across different `GXVtxFmt` values
  - [ ] #111 — guard `GX_LINESTRIP` zero/short vertex counts
  - [ ] #112 — make unsupported indexed XF loads visible in Release builds
- [x] Hardware-unblock M3 by proving the current black-screen runtime remains active through 13,918 VI retraces
- [x] Hardware-present an isolated loaderless NVK / `VK_NN_vi_surface` clear frame on real Switch (#162/#165)
- [x] Present a simple Vulkan triangle on the proven VI/NVK swapchain — **hardware PASS: visible triangle on real Switch (2026-09-18); SD report bug fixed separately**
- [x] Prove Dawn/WebGPU over the proven Vulkan/NVK path — **hardware PASS: 1280x720 surface, first present, 1,507-frame stable loop**
- [x] Prove a WGSL shader + Dawn graphics pipeline + triangle on real Switch — **hardware PASS: visible RGB triangle + clean explicit teardown**
- [x] Select the native Switch graphics strategy compatible with WiiCompiled/Aurora — **Aurora GX → Dawn/WebGPU → Vulkan/NVK is the primary path; Deko3D remains fallback**
- [x] Present an isolated Aurora GX triangle on real Switch — **hardware PASS: first Aurora GX triangle + 563-frame active loop + clean teardown**
- [x] Prove pinned WiiCompiled `HleFifoWrite` → Aurora GX with a fabricated Nintendo-data-free FIFO stream — **hardware PASS: exact pin, raw-direct path, 1,435-frame stable loop**
- [ ] Replace the temporary GX FIFO sink with a real GX → Switch command/backend path — **separate RMCP01 rendered fast-track is hardware-running; game has not produced drawable FIFO work yet**
- [x] Render first native Switch clear frame
- [ ] GX command path functional
- [ ] shader/pipeline cache strategy
- [ ] 720p handheld / 1080p docked policy

The upstream GX audit behind issues #109–#112 is recorded in `docs/UPSTREAM_GX_AUDIT_2026-09-13.md`. These are shared decoder/runtime concerns and must be separated from Switch-backend-specific failures during M3.

> The stable #117 fast-track intentionally keeps its FIFO sink as a control baseline. The first run after #200 hardware-proves `GXSetVtxAttrFmt` and preserves scheduler recovery; this specific run has `TaskThread::run hits = 0`, while the priority-24 worker still reaches guest-fiber entry. The renderer remains at nine FIFO writes with no drawable work. The new first durable blocker is PAL `GXSetNumChans` at `0x8017054C` with observed `r3 = 1`. The user-visible return to hbmenu / Switch error matches the deliberate unsupported-boundary `std::abort()` path. No DVD read, display list, drawable RMCP01 FIFO work, `GXCopyDisp`, or present has been observed yet.

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
- [x] hardware-cross the observed post-main scheduler/input/time/SC sequence through `OSWakeupThread`
- [x] reach sustained translated execution in the EGG display subsystem
- [x] classify the sustained black-screen path as an active VI/post-retrace loop
- [x] attribute the #188 first-fiber crash at `0x8042A680 -> 0x8024373C` to runtime-boundary VI polling clobbering the interrupted `CpuContext`
- [x] hardware-validate register-isolated VI polling and restore the first guest-fiber blocking path
- [x] hardware-validate the `0x90112660` fiber-aware `VIWaitForRetrace` starvation fix: worker parks on `0x80386BC0`, scheduler returns to main
- [x] hardware-cross PAL `OSSendMessage` (`0x801A735C`) after scheduler recovery
- [x] hardware-cross PAL `GXDrawDone` (`0x8016EAB0`) using the pinned draw-done bookkeeping and Aurora FIFO drain
- [x] hardware-cross virtual `EGG::TaskThread::run` (`0x80242D7C`) with the explicit hit counter
- [x] hardware-cross PAL `GXSetProjection` (`0x8017301C`) using the pinned guest-matrix -> Aurora contract
- [x] hardware-cross PAL `GXSetViewport` (`0x801733B4`) using PPC f1..f6 and the pinned Aurora viewport contract
- [x] hardware-cross PAL `GXSetScissor` (`0x80173430`) using pinned guest GXData bookkeeping plus Aurora scissor
- [x] hardware-cross PAL `GXLoadPosMtxImm` (`0x8017310C`) using the pinned 3x4 guest-matrix -> Aurora contract
- [x] hardware-cross PAL `GXSetCurrentMtx` (`0x80173214`) using the pinned `r3` matrix-id -> Aurora contract
- [x] hardware-cross PAL `GXClearVtxDesc` (`0x8016DC34`) using pinned descriptor-state reset + Aurora contract
- [x] hardware-cross PAL `GXSetVtxDesc` (`0x8016D3A4`) using pinned HLE descriptor bookkeeping + Aurora direct-stream contract
- [x] hardware-cross PAL `GXSetVtxAttrFmt` (`0x8016DC68`) using pinned HLE format bookkeeping + Aurora contract
- [ ] hardware-cross PAL `GXSetNumChans` (`0x8017054C`) using pinned `r3 -> u8 -> Aurora` contract
- [ ] continue post-main initialization through system/resource initialization
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
