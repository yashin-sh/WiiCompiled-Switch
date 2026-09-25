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
- `HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md` — first #192 hardware run, TaskThread proof caveat, and PAL `GXSetProjection` frontier
- `HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md` — #193 hardware PASS for TaskThread/projection and new PAL `GXSetViewport` frontier
- `HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md` — #194 hardware PASS for viewport and new PAL `GXSetScissor` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md` — #195 hardware PASS for scissor and new PAL `GXLoadPosMtxImm` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md` — #196 hardware PASS for `GXLoadPosMtxImm`, TaskThread re-proof, and new PAL `GXSetCurrentMtx` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md` — #197 hardware PASS for `GXSetCurrentMtx` and new PAL `GXClearVtxDesc` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md` — #198 hardware PASS for `GXClearVtxDesc`, TaskThread re-proof, first post-bootstrap GX state FIFO byte, and new PAL `GXSetVtxDesc` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md` — #199 hardware PASS for `GXSetVtxDesc` and new PAL `GXSetVtxAttrFmt` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md` — #200 hardware PASS for `GXSetVtxAttrFmt`, deliberate-abort behavior, and new PAL `GXSetNumChans` frontier
- `HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md` — #201 hardware PASS for `GXSetNumChans`, TaskThread re-proof, and new PAL `GXSetChanMatColor` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md` — rendered hardware progression beyond `GXSetChanCtrl` and new PAL `GXSetNumTexGens` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md` — rendered hardware progression beyond `GXSetNumTexGens` and new PAL `GXSetNumIndStages` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md` — rendered hardware progression beyond `GXSetNumIndStages`, eleven FIFO writes, and new PAL `GXSetNumTevStages` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md` — rendered hardware progression beyond `GXSetNumTevStages` and new PAL `GXSetTevOp` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md` — rendered hardware progression beyond `GXSetTevOp` and new PAL `GXSetTevOrder` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md` — rendered hardware progression beyond `GXSetTevOrder` and new PAL `GXSetBlendMode` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md` — rendered hardware progression beyond `GXSetBlendMode` and new PAL `GXSetColorUpdate` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_ALPHA_UPDATE_FRONTIER.md` — rendered hardware progression beyond `GXSetColorUpdate` and new PAL `GXSetAlphaUpdate` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_Z_MODE_FRONTIER.md` — rendered hardware progression beyond `GXSetAlphaUpdate` and new PAL `GXSetZMode` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_CULL_MODE_FRONTIER.md` — rendered hardware progression beyond `GXSetZMode` and new PAL `GXSetCullMode` frontier
- `HARDWARE_RESULTS_2026-09-21_GX_BEGIN_FRONTIER.md` — rendered hardware progression beyond `GXSetCullMode` to the first PAL `GXBegin` draw-primitive frontier
- `HARDWARE_RESULTS_2026-09-21_GX_SET_COPY_FILTER_FRONTIER.md` — first real FIFO work preserved, `endRender` hardware-crossed, and new PAL `GXSetCopyFilter` copy-path frontier
- `HARDWARE_RESULTS_2026-09-21_FIRST_RMCP01_PRESENT_GX_FLUSH_FRONTIER.md` — first successful game-facing RMCP01 GPU present (`hadWork=1`) and new PAL `GXFlush` frontier
- `HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md` — `GXFlush` hardware-crossed with 23 successful presents; later TaskThread indirect target equals the worker guest stack pointer and requires job-field diagnostics
- `HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md` — TaskThread telemetry proves the received “job” aliases the worker stack; next gate is producer-send vs queue-buffer attribution
- `HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md` — proves `TaskThread::request` sends the valid `mJobs[0]` pointer; current frontier is the exact `OSReceiveMessage` output-slot clobber phase
- `HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md` — phase trace proves the slot is correct until the wakeup call boundary and defines the minimal fix: no VI retrace polling while guest interrupts are disabled
- `HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md` — hardware-validates the VI interrupt-mask fix, records the first real `/Boot/Strap/eu/English.szs` read-pass, and moves the frontier to `SELECTTHREAD_IDLE_POLL`
- `HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md` — identifies default-thread queue `0x804294A4` as `AsyncDisplay + 0x58` and attributes the required idle wake to VI `postVRetrace()`
- `HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md` — hardware-validates the VI-only AsyncDisplay idle wake, recovers real FIFO/present work, and moves the exact resource frontier to pinned `EGG::Decomp::decodeSZS (0x80218C2C)`
- `HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md` — proves full `English.szs` SZS expansion and moves the exact rendered frontier to `GXInitTexObj (0x801707F8)`
- `HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md` — hardware-crosses `GXInitTexObj`, preserves the real FIFO/present path, and moves the frontier to pinned `NAND_IOS_Open (0x801938F8)` with path/mode diagnostics only
- `HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md` — identifies `/dev/net/kd/request`, mode 0, as the exact IOS_Open request and constrains the next candidate to pinned KD device-handle allocation only
- `HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md` — hardware-crosses the KD open, captures fd/cmd/in/out for pinned `IOS_Ioctl (0x80194290)` command 2, and defines the exact one-shot Boot-phase `-42` output candidate
- `HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md` — hardware-crosses that first KD command-2 probe and exposes pinned `IOS_Close (0x80193AD8)` with the same fd 2000
- `HARDWARE_RESULTS_2026-09-25_IOS_CLOSE_CROSSED_GX_LOAD_TEX_OBJ_FRONTIER.md` — hardware-crosses fd-2000 IOS_Close, records the first `StaticR.rel` read / `RKSystem::run` progress, and moves the frontier to `GXLoadTexObj (0x80170F2C)` diagnostics
- `HARDWARE_RESULTS_2026-09-25_GX_LOAD_TEX_OBJ_DESCRIPTOR_CAPTURED.md` — captures the exact first GXLoadTexObj descriptor and defines the strict one-descriptor Aurora bind candidate
- `HARDWARE_RESULTS_2026-09-25_GX_LOAD_TEX_OBJ_CROSSED_TEXCOORDGEN2_FRONTIER.md` — hardware-crosses that first texture load and moves the exact frontier to GXSetTexCoordGen2
- `HARDWARE_RESULTS_2026-09-25_TEXCOORDGEN2_CROSSED_STRAP_CHECK_INPUT_FRONTIER.md` — hardware-crosses GXSetTexCoordGen2, records sustained 60-frame rendering, and moves the exact frontier to StrapScene::CheckInput
- `HARDWARE_RESULTS_2026-09-25_STRAP_CHECK_INPUT_CROSSED_STATICR_REL_PROLOG_FRONTIER.md` — hardware-crosses StrapScene::CheckInput, records 61 successful presents / 0 failures, and attributes the first StaticR native-wrapper frontier to RelProlog at 0x8055531C
- `HARDWARE_RESULTS_2026-09-25_STATICR_REL_PROLOG_CROSSED_OS_DETACH_THREAD_FRONTIER.md` — hardware-crosses StaticR RelProlog and moves the frontier to OSDetachThread diagnostics
- `HARDWARE_RESULTS_2026-09-25_OS_DETACH_THREAD_LIVE_PATH.md` — captures the exact WAITING/already-detached/empty-join TaskThread state and defines the strict first OSDetachThread candidate
- `HARDWARE_RESULTS_2026-09-25_OS_DETACH_THREAD_CROSSED_OS_CANCEL_THREAD_FRONTIER.md` — hardware-crosses OSDetachThread and moves the exact frontier to OSCancelThread diagnostics
- `HARDWARE_RESULTS_2026-09-21_FIRST_RMCP01_FIFO_WORK_END_RENDER_FRONTIER.md` — `GXBegin` hardware-crossed, first real RMCP01 FIFO/Aurora render work, and new `EGG::AsyncDisplay::endRender` frontier

## Blocker notes

- `fast-track-blockers/` — blocker-specific mapping, pinned semantics and fix notes

For the current project status, use `../README.md`, `../ROADMAP.md`,
`FAST_TRACK_VALIDATION_POLICY.md`, `M2_RUNTIME_BOOTSTRAP.md`, and issue #117.

The latest 2026-09-25 hardware evidence preserves the proven local
`English.szs`/StaticR resource path and sustained Aurora/Dawn/NVK rendering.
The run reaches 17,800 translated dispatches, 765 RMCP01 FIFO writes and 61
successful presents with zero failures. It hardware-crosses the merged
`StrapScene::CheckInput (0x800077C8)` seam and stops at
`INDIRECT_CALL_MISS 0x8055531C`.

Pinned RMCP01/WiiCompiled attribution identifies `0x8055531C` as StaticR.rel
`RelProlog`. Pinned WiiCompiled registers a native winner that brackets the
retained original `func_8055531C` with host mod-initializer phases. The base
Switch product has no mod data-patch registrants, so the current candidate
routes only this exact native boundary to the already-generated original
RelProlog and does not fabricate REL relocation/loading behavior.

The earlier real FIFO / `GXCopyDisp` / repeated-present / `GXFlush` proof
remains valid. Visual correctness remains a separate milestone. Dated hardware
result files are historical evidence and intentionally retain the frontier
wording that was correct when each run was captured.


Latest follow-up: merged PR #242 is hardware-crossed. The run reaches 24,544
translated dispatches, 269 StaticR dispatches, 1,240 RMCP01 FIFO writes and 84
successful presents with zero failures. The new exact blocker is pinned
`OSDetachThread (0x801AA4EC)` with `r3=0x901187C0`, the TaskThread OSThread.
Because the pinned function has a state-dependent MORIBUND cleanup branch, the
current patch captures the exact live OSThread fields before implementing any
scheduler mutation.
