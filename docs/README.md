# Documentation index

## Architecture / runtime

- `ARCHITECTURE.md` — target architecture
- `M1_PORTABILITY_AUDIT.md` — WiiCompiled -> Horizon/libnx audit
- `M1_RISK_MATRIX.md` — prioritized technical risks
- `M1_TEST_PLAN.md` — hardware probes required before deeper integration
- `M2_VM_PROBE.md` — Horizon guest virtual-memory probe
- `M2_CONTEXT_PROBE.md` — AArch64 cooperative-context probe
- `M2_HORIZON_GUESTFLAT.md` — heap-backed checked GuestFlat integration
- `M2_RUNTIME_BOOTSTRAP.md` — authoritative current runtime/bootstrap and post-main fast-track status
- `M2_SYNTHETIC_PRODUCT_PROBE.md` — hardware-validated weak/strong translated-product link seam
- `M2_DATA_INIT_HANDOFF.md` — guarded data-section initialization and local-only generation path
- `M2_LOCAL_DATA_INIT_HARDWARE_PASS.md` — real-Switch PASS for user-owned RMCP01 generated data sections
- `M2_FUNCTION_SHARD_LINK.md` — local translated-function compile/link-only boundary
- `TRANSLATED_PRODUCT_BOUNDARY.md` — build-time translated-product seam, local-only product policy and current execution milestone
- `GRAPHICS_NOTES.md` — graphics backend notes and M3 direction
- `M3_VULKAN_CLEAR_PROBE.md` — isolated loaderless NVK / `VK_NN_vi_surface` clear-frame hardware probe for #162
- `M3_VULKAN_TRIANGLE_PROBE.md` — isolated shader/pipeline/rasterisation probe on the hardware-proven NVK/VI swapchain
- `M3_DAWN_CLEAR_PROBE.md` — isolated Dawn/WebGPU clear/present probe over the proven Vulkan/NVK/VI path
- `M3_DAWN_TRIANGLE_PROBE.md` — isolated WGSL shader/pipeline/triangle probe over the hardware-proven Dawn path
- `M3_AURORA_GX_PROBE.md` — isolated Aurora GX API/FIFO triangle over the hardware-proven Dawn/NVK path
- `M3_HLE_FIFO_AURORA_PROBE.md` — bytewise fabricated FIFO through pinned WiiCompiled `HleFifoWrite` into Aurora GX
- `M3_RMCP01_RENDERED_FAST_TRACK.md` — first local game-facing rendered fast-track using the proven FIFO/Aurora/Dawn/NVK path
- `WII_PORTING_REFERENCE_AUDIT_2026-09-16.md` — reference/tooling hierarchy and licensing notes
- `STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md` — targeted `new-coke/strikers` / Aurora audit for #154 DVD/FST and #4/#162 first-frame work

## Hardware evidence

- `HARDWARE_RESULTS_2026-09-09.md` — first recorded hardware evidence
- `HARDWARE_RESULTS_2026-09-10.md` — hardware-validated runtime bootstrap, memory and translated-product results
- `HARDWARE_DATA_INIT_PASS_2026-09-10.md` — real-Switch synthetic data-init handoff PASS
- `HARDWARE_RESULTS_2026-09-12.md` — translated startup blocker progression on real Switch
- `HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md` — PAL Mario Kart Wii `main()` reached on real Switch
- `HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md` — ordered post-main progression through MEM2, mutex/thread and GXInit boundaries
- `HARDWARE_RESULTS_2026-09-15_OS_CREATE_THREAD.md` — first hardware thread-creation frontier
- `HARDWARE_RESULTS_2026-09-16_OS_INIT_MESSAGE_QUEUE.md` — message-queue frontier
- `HARDWARE_RESULTS_2026-09-16_SELECT_THREAD.md` — guest scheduler selection frontier
- `HARDWARE_RESULTS_2026-09-16_OS_LOAD_CONTEXT.md` — guest context restore frontier
- `HARDWARE_RESULTS_2026-09-16_OS_RECEIVE_MESSAGE.md` — blocking receive frontier
- `HARDWARE_RESULTS_2026-09-16_OS_SLEEP_THREAD.md` — guest wait/sleep frontier
- `HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md` — HostContext guest-thread continuation proof
- `HARDWARE_RESULTS_2026-09-16_WPAD_INIT.md` — WPAD initialization frontier
- `HARDWARE_RESULTS_2026-09-16_WPAD_DPD_SENSITIVITY.md` — WPAD DPD sensitivity frontier
- `HARDWARE_RESULTS_2026-09-16_WPAD_GET_STATUS.md` — WPAD status frontier
- `HARDWARE_RESULTS_2026-09-16_WPAD_CONTROL_MOTOR.md` — WPAD motor frontier
- `HARDWARE_RESULTS_2026-09-16_PAD_INIT.md` — PAD initialization frontier
- `HARDWARE_RESULTS_2026-09-16_OS_GET_TIME.md` — time-base getter frontier
- `HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md` — power callback frontier
- `HARDWARE_RESULTS_2026-09-16_SC_GET_PRODUCT_AREA.md` — console product-area frontier
- `HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md` — scheduler wakeup frontier
- `HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md` — 37,148-dispatch sustained post-main run and EGG AsyncDisplay attribution
- `HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md` — 126,563-dispatch run proving active `PostRetraceCallback` / VI retrace progression
- `HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md` — real-Switch loaderless NVK / `VK_NN_vi_surface` changing-color clear-frame PASS
- `HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md` — real-Switch shader/pipeline/rasterisation triangle PASS; records the separate SD-report fix
- `HARDWARE_RESULTS_2026-09-18_M3_DAWN_CLEAR.md` — real-Switch Dawn/WebGPU clear/present PASS with 1,507-frame stable loop
- `HARDWARE_RESULTS_2026-09-18_M3_DAWN_TRIANGLE.md` — real-Switch WGSL/Dawn graphics-pipeline triangle PASS with clean explicit teardown
- `HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md` — real-Switch Aurora GX triangle PASS with 563-frame active loop and clean teardown
- `HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md` — exact pinned `HleFifoWrite` → Aurora GX PASS with 1,435-frame active loop
- `HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md` — FST publication PASS, #185 DVD-read non-reachability, and later priority-6 OSThread `0x90112660` frontier
- `HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md` — #188 first-fiber register-clobber regression and #189 correction gate
- `HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md` — #189 scheduler recovery PASS and new PAL `OSSendMessage` blocker
- `HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md` — #190 OSSendMessage hardware PASS and new PAL `GXDrawDone` blocker
- `HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md` — #191 GXDrawDone hardware PASS and priority-24 ResourceManager `TaskThread::run` frontier

## Blocker notes

- `fast-track-blockers/` — blocker-specific mapping, pinned semantics and fix notes

For the current project status, use `../README.md`, `../ROADMAP.md`, `M2_RUNTIME_BOOTSTRAP.md`, and issue #117. Dated hardware result files are historical evidence and intentionally retain the frontier wording that was correct when each run was captured.
