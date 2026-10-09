# Documentation index

- [KD accepted and projection-vector boundary](HARDWARE_KD_PROJECTION_2026-10-09.md) — post-resume probe/close accepted; shared projection save/restore candidate under validation

- [Recognizable Switch image and desktop replay](HARDWARE_NONBLACK_REPLAY_2026-10-09.md) — Wiimote safety page retained at frame 4, 90-frame replay validated; later black interval and KD acceptance remain open

- [Sampled SD diagnostics and KD post-resume stop](HARDWARE_KD_POST_RESUME_2026-10-09.md) — early 7.5–9.3 Hz, later stalls and a diagnosed fifth KD request; phase correction pending console

- [Measured capture-disabled startup](HARDWARE_PRESENT_RATE_2026-10-09.md) — Wiimote warning then black, about 1.18 Hz; diagnostic SD sampling correction pending hardware
- [Previous console trial and capture control](HARDWARE_CAPTURE_CONTROL_2026-10-09.md) — operator reports boot images/low FPS; saved black checkpoints and replay budget failure
- [Texture families, XFB presentation and later-frame replay](GX_TEXTURE_FAMILY_PRESENT_REPLAY.md) — address-independent loading crossed the prior guard; follow-up capture/control trial pending

- [Validated Switch first/latest GPU images](HARDWARE_SURFACE_CHECKPOINT_2026-10-08.md) — frame 102 checkpoint and SD replacement pass; RGB remains black
- [First actual Switch GPU image](HARDWARE_SURFACE_FIRST_IMAGE_2026-10-08.md) — preceding partial result; latest PNG replacement failed
- [Build and hardware diagnostics](BUILD.md)
- [Status history moved from the README](STATUS_LOG.md)
- [Desktop GX capture/replay prototype](DESKTOP_GX_REPLAY.md)
- [Actual Switch surface-image diagnostic](SWITCH_FRAME_DUMP.md) — first/latest readback and SD checkpoint hardware-validated
- [Report terminology](REPORT_TERMS.md) — definitions for reading detailed evidence
- [Next-pass I4 36×32 correction](GX_MII_I4_36X32_NEXT_PASS_LOAD_2026-10-08.md) — prior load/helper return supported by later caller progression
- [Next-pass RGB5A3 return and I4 36×32 frontier](HARDWARE_NEXT_PASS_RGB5A3_RETURN_2026-10-08.md) — scoped caller inference; initial black PNG unchanged
- [First real Switch capture and desktop replay](HARDWARE_FIRST_FRAME_REPLAY_2026-10-08.md) — complete initial frame, reproducible black PNG; later textured frames pending
- [Second next-pass Mii I4 return and RGB5A3 frontier](HARDWARE_SECOND_NEXT_PASS_I4_RETURN_2026-10-08.md) — scoped caller inference; first-frame PNG unchanged
- [Next-pass Mii RGB5A3 correction](GX_MII_RGB5A3_NEXT_PASS_LOAD_2026-10-08.md) — exact observed 44×32 object; observed native/helper return accepted
- [Second next-pass Mii I4 correction](GX_MII_I4_SECOND_NEXT_PASS_LOAD_2026-10-08.md) — exact captured identity; observed native/helper return accepted
- [First relocated Mii I4 returned; second-object frontier](HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_RELOCATED_DATA_FRONTIER.md)
- [Captured second relocated Mii I4 correction](GX_MII_I4_SECOND_RELOCATED_LOAD_2026-10-07.md)
- [First Mii I4 load with relocated data; next-pass return unconfirmed](HARDWARE_RESULTS_2026-10-07_MII_I4_RELOCATED_DATA_FRONTIER.md)
- [Captured relocated first Mii I4 source correction](GX_MII_I4_RELOCATED_LOAD_2026-10-07.md)
- [I4 16×16 return and next Mii pass frontier](HARDWARE_RESULTS_2026-10-07_MII_I4_32X64_NEXT_PASS_FRONTIER.md)
- [Captured next-pass I4 32×64 correction](GX_MII_I4_32X64_NEXT_PASS_LOAD_2026-10-07.md)
- [Second RGB5A3 38×32 return and I4 16×16 frontier](HARDWARE_RESULTS_2026-10-07_MII_I4_16X16_LOAD_FRONTIER.md)
- [Captured Mii I4 16×16 correction](GX_MII_I4_16X16_LOAD_2026-10-07.md)
- [First RGB5A3 38×32 return and second-object frontier](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_38X32_SECOND_LOAD_FRONTIER.md)
- [Second captured RGB5A3 38×32 correction](GX_MII_RGB5A3_38X32_SECOND_LOAD_2026-10-07.md)
- [Second I4 36×32 return and RGB5A3 38×32 frontier](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_38X32_LOAD_FRONTIER.md)
- [Captured RGB5A3 38×32 correction](GX_MII_RGB5A3_38X32_LOAD_2026-10-07.md)

- [First I4 36×32 return and second-object frontier](HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_SECOND_LOAD_FRONTIER.md)
- [Second captured I4 36×32 object correction](GX_MII_I4_36X32_SECOND_LOAD_2026-10-07.md)

- [RGB5A3 return and I4 36×32 frontier](HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_LOAD_FRONTIER.md)
- [Captured I4 36×32 load correction](GX_MII_I4_36X32_LOAD_2026-10-07.md)

- [Second I4 return and RGB5A3 frontier](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_LOAD_FRONTIER.md)
- [Captured RGB5A3 load correction](GX_MII_RGB5A3_LOAD_2026-10-07.md)

## Architecture / runtime

- [GX_VIEWPORT_STATE_2026-10-06.md](GX_VIEWPORT_STATE_2026-10-06.md) — guest viewport and bounded depth dependency; 22 suites, code-head CI/private build pass, SD readback verified, hardware pending
- [HARDWARE_RESULTS_2026-10-06_PIX_MODE_SYNC_VIEWPORT_FRONTIER.md](HARDWARE_RESULTS_2026-10-06_PIX_MODE_SYNC_VIEWPORT_FRONTIER.md) — latest verified run: PixModeSync returned, viewport getter frontier; visual pending
- [GX_PIX_MODE_SYNC_2026-10-06.md](GX_PIX_MODE_SYNC_2026-10-06.md) — next synchronization candidate; 21 suites, five workflows and private build pass, observed console return accepted; merged #318
- [HARDWARE_RESULTS_2026-10-06_GX_COPY_TEX_PIX_MODE_SYNC_FRONTIER.md](HARDWARE_RESULTS_2026-10-06_GX_COPY_TEX_PIX_MODE_SYNC_FRONTIER.md) — preceding run: native copy returned, PixModeSync frontier, black then error
- [GX_COPY_TEX_2026-10-06.md](GX_COPY_TEX_2026-10-06.md) — current bounded native copy/cache lifetime candidate; twenty suites, five workflows and private build pass, verified native return on console; GPU pixels pending
- [HARDWARE_RESULTS_2026-10-06_TEXTURE_COPY_CONFIG_COPY_TEX_FRONTIER.md](HARDWARE_RESULTS_2026-10-06_TEXTURE_COPY_CONFIG_COPY_TEX_FRONTIER.md) — preceding verified run: Clamp/Src/Dst returned, GXCopyTex frontier; visual result pending
- [GX_TEXTURE_COPY_CONFIG_2026-10-06.md](GX_TEXTURE_COPY_CONFIG_2026-10-06.md) — all three observed configuration returns accepted; GXCopyTex and cache lifetime remain open
- [HARDWARE_RESULTS_2026-10-06_MII_FOG_COPY_CLAMP_FRONTIER.md](HARDWARE_RESULTS_2026-10-06_MII_FOG_COPY_CLAMP_FRONTIER.md) — latest console result: Mii Fog returned; Clamp(3) frontier; 36 reports verified, visual pending
- [GX_FOG_DEGENERATE_2026-10-05.md](GX_FOG_DEGENERATE_2026-10-05.md) — Mii texture Fog `(1,1,0,0)` tuple; all gates pass, exact observed tuple console return accepted
- [HARDWARE_RESULTS_2026-10-05_PAD_RESET_MII_FOG_FRONTIER.md](HARDWARE_RESULTS_2026-10-05_PAD_RESET_MII_FOG_FRONTIER.md) — preceding result: PADReset mask returned; Mii texture Fog frontier, 36 reports verified, black then error
- [PAD_RESET_2026-10-05.md](PAD_RESET_2026-10-05.md) — pinned ignored-mask success return; 5,223 cases, all gates pass; observed mask `0x70000000` console return accepted
- [HARDWARE_RESULTS_2026-10-05_PAD_CONTROL_MOTOR_PAD_RESET_FRONTIER.md](HARDWARE_RESULTS_2026-10-05_PAD_CONTROL_MOTOR_PAD_RESET_FRONTIER.md) — preceding result: motor `(0,2)` returned; PADReset mask `0x70000000` frontier; 35 reports verified, visual observation pending
- [PAD_CONTROL_MOTOR_2026-10-05.md](PAD_CONTROL_MOTOR_2026-10-05.md) — absent-actuator void return; local/CI/private-build gates passed, observed channel-0 / command-2 console return accepted
- [HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md](HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md) — preceding result: KPAD count-1 polling returned, motor frontier, black then error
- [KPAD_UNIFIED_STATUS_2026-10-04.md](KPAD_UNIFIED_STATUS_2026-10-04.md) — absent-remote samples, 8,987 cases, five workflows / six jobs and private NRO passed; observed count-1 polling hardware-crossed

- [HARDWARE_RESULTS_2026-10-04_WPAD_PROBE_KPAD_UNIFIED_FRONTIER.md](HARDWARE_RESULTS_2026-10-04_WPAD_PROBE_KPAD_UNIFIED_FRONTIER.md) — preceding console result: WPADProbe returned on channel 0; KPAD unified status frontier, black then error

- [WPAD_PROBE_2026-10-04.md](WPAD_PROBE_2026-10-04.md) — current absent-Wiimote candidate, 556 cases, six mutations, five workflows / six jobs and private build passed; channel 0 hardware return accepted; KPAD unified status is next

- [HARDWARE_RESULTS_2026-10-04_PAD_READ_WPAD_PROBE_FRONTIER.md](HARDWARE_RESULTS_2026-10-04_PAD_READ_WPAD_PROBE_FRONTIER.md) — preceding console result: PADRead returned, connected port 0, translated clamping and WPADProbe frontier

- [PAD_READ_2026-10-04.md](PAD_READ_2026-10-04.md) — real Switch single-controller PADRead candidate, four-slot encoding, 65,563 host cases and passing workflows/private build; console PADRead return accepted; per-button hardware tests open

- [HARDWARE_RESULTS_2026-10-04_SPHERE_PAD_READ_FRONTIER.md](HARDWARE_RESULTS_2026-10-04_SPHERE_PAD_READ_FRONTIER.md) — preceding console result: both sphere variants returned; PADRead frontier, black then error
- [HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_SPHERE_FRONTIER.md](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_SPHERE_FRONTIER.md) — preceding console result: empty-update SU processing, Begin/End returned, later 64-byte length; GXDrawSphere `(4,8)` frontier

- [HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md) — preceding console result: Begin refuses guest SU dirty bit 0 before recording; native list inactive

- [HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md](HARDWARE_RESULTS_2026-10-03_DEPTH_LOD_DISPLAY_LIST_FRONTIER.md) — preceding accepted depth LOD return; GXBeginDisplayList 16 KiB buffer frontier, user-confirmed black screen and error
- [GX_DEPTH_LOD_2026-10-03.md](GX_DEPTH_LOD_2026-10-03.md) — fixes the missing full Z24X8 format in LOD validation; local/native contracts, all workflows and private build pass; observed LOD console return accepted
- [HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-10-03_FOG_Z_COMP_DEPTH_LOD_FRONTIER.md) — preceding accepted Fog/ZCompLoc and pixel-setup returns; native depth-texture init passes, LOD validation is the next stop; that run’s visual observation was not supplied
- [GX_FOG_Z_COMP_2026-10-03.md](GX_FOG_Z_COMP_2026-10-03.md) — bounded Fog/ZCompLoc, passing host/native contracts, workflows and private build; observed console returns accepted
- [HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md](HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md) — preceding accepted AlphaCompare return and captured Fog arguments; user-confirmed black screen and error at exit

- [GX_ALPHA_COMPARE_2026-10-03.md](GX_ALPHA_COMPARE_2026-10-03.md) — next bounded bridge, host validity-flag semantics and Fog diagnostics

- [HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md](HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md) — preceding accepted twelve color/table calls; AlphaCompare boundary, black output and crash

- [PORT_AUDIT_2026-10-03.md](PORT_AUDIT_2026-10-03.md) — CI/scripts/runtime audit, corrections and remaining validation limits
- [ARCHITECTURE.md](ARCHITECTURE.md) — implemented architecture, hardware proof and remaining risks
- [M1_PORTABILITY_AUDIT.md](M1_PORTABILITY_AUDIT.md) — WiiCompiled -> Horizon/libnx audit
- [M1_RISK_MATRIX.md](M1_RISK_MATRIX.md) — prioritized technical risks
- [M1_TEST_PLAN.md](M1_TEST_PLAN.md) — hardware probes required before deeper integration
- [M2_VM_PROBE.md](M2_VM_PROBE.md) — Horizon guest virtual-memory probe
- [M2_CONTEXT_PROBE.md](M2_CONTEXT_PROBE.md) — AArch64 cooperative-context probe
- [M2_HORIZON_GUESTFLAT.md](M2_HORIZON_GUESTFLAT.md) — heap-backed checked GuestFlat integration
- [M2_RUNTIME_BOOTSTRAP.md](M2_RUNTIME_BOOTSTRAP.md) — runtime/bootstrap foundation and historical bring-up context; current frontier links are kept at the top
- [M2_SYNTHETIC_PRODUCT_PROBE.md](M2_SYNTHETIC_PRODUCT_PROBE.md) — hardware-validated weak/strong translated-product link seam
- [M2_DATA_INIT_HANDOFF.md](M2_DATA_INIT_HANDOFF.md) — guarded data-section initialization and local-only generation path
- [M2_LOCAL_DATA_INIT_HARDWARE_PASS.md](M2_LOCAL_DATA_INIT_HARDWARE_PASS.md) — real-Switch PASS for user-owned RMCP01 generated data sections
- [M2_FUNCTION_SHARD_LINK.md](M2_FUNCTION_SHARD_LINK.md) — local translated-function compile/link-only boundary
- [TRANSLATED_PRODUCT_BOUNDARY.md](TRANSLATED_PRODUCT_BOUNDARY.md) — build-time translated-product seam, local-only product policy and current rendered execution milestone
- [GRAPHICS_NOTES.md](GRAPHICS_NOTES.md) — graphics backend notes and M3 direction
- [M3_VULKAN_CLEAR_PROBE.md](M3_VULKAN_CLEAR_PROBE.md) — isolated loaderless NVK / `VK_NN_vi_surface` clear-frame hardware probe for #162
- [M3_VULKAN_TRIANGLE_PROBE.md](M3_VULKAN_TRIANGLE_PROBE.md) — isolated shader/pipeline/rasterisation probe on the hardware-proven NVK/VI swapchain
- [M3_DAWN_CLEAR_PROBE.md](M3_DAWN_CLEAR_PROBE.md) — isolated Dawn/WebGPU clear/present probe over the proven Vulkan/NVK/VI path
- [M3_DAWN_TRIANGLE_PROBE.md](M3_DAWN_TRIANGLE_PROBE.md) — isolated WGSL shader/pipeline/triangle probe over the hardware-proven Dawn path
- [M3_AURORA_GX_PROBE.md](M3_AURORA_GX_PROBE.md) — isolated Aurora GX API/FIFO triangle over the hardware-proven Dawn/NVK path
- [M3_HLE_FIFO_AURORA_PROBE.md](M3_HLE_FIFO_AURORA_PROBE.md) — bytewise fabricated FIFO through pinned WiiCompiled `HleFifoWrite` into Aurora GX
- [M3_RMCP01_RENDERED_FAST_TRACK.md](M3_RMCP01_RENDERED_FAST_TRACK.md) — first local game-facing rendered fast-track using the proven FIFO/Aurora/Dawn/NVK path
- [WII_PORTING_REFERENCE_AUDIT_2026-09-16.md](WII_PORTING_REFERENCE_AUDIT_2026-09-16.md) — historical reference/tooling hierarchy and licensing audit
- [RMCP01_GX_TEXTURE_CALLSITE_SCAN.md](RMCP01_GX_TEXTURE_CALLSITE_SCAN.md) — local-only DOL/REL or MEM1 scan for direct GX texture-object callsites, attributed with public doldecomp/Ghidra metadata
- [STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md](STRIKERS_AURORA_REFERENCE_AUDIT_2026-09-17.md) — historical `new-coke/strikers` / Aurora audit for #154 DVD/FST and #4/#162 first-frame work

## Validation / analysis method

- [GX_DRAW_SPHERE_2026-10-04.md](GX_DRAW_SPHERE_2026-10-04.md) — bounded recording bridge, native geometry/state tests and verified float conversion correction; all workflows/private build pass; both variants return on console

- [GX_SU_STATE_2026-10-04.md](GX_SU_STATE_2026-10-04.md) — handles observed guest SU dirty bit with actual native emission and selective guest shadow publication; console accepts the empty-update branch and Begin/End return

- [FAST_TRACK_VALIDATION_POLICY.md](FAST_TRACK_VALIDATION_POLICY.md) — exact-candidate CI, private rendered gate, audited GX batches and hardware acceptance
- [RMCP01_DISCOVERY_SCAN.md](RMCP01_DISCOVERY_SCAN.md) — whole-product direct-call coverage and first-hit runtime evidence
- [FAST_TRACK_LOG_BUNDLES.md](FAST_TRACK_LOG_BUNDLES.md) — compact/full bundles, manifests, run identity and stale-report limits
- [RMCP01_ADDRESS_ATTRIBUTION.md](RMCP01_ADDRESS_ATTRIBUTION.md) — public address metadata, pinned semantics and local DTK follow-up
- [RMCP01_FRONTIER_FORECAST.md](RMCP01_FRONTIER_FORECAST.md) — static look-ahead and its limits

- [GX display-list candidate](GX_DISPLAY_LIST_2026-10-03.md) — coordinated Begin/End, bounded native/guest FIFO and context restoration; SU-corrected console run accepts Begin and at least one End; replay pending

## Hardware evidence

- [TEV scalar hardware result](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md) — six setters return on stages 0..15; KColor pointer frontier, black screen and error at exit
- [Earlier audit result](HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md) — normal-path non-regression at the preceding Direct frontier; error-path limits
- [Coordinate-to-TEV result](HARDWARE_RESULTS_2026-10-03_DISCOVERY_GX_TEV_DIRECT_FRONTIER.md) — all eight coordinate triples return; TEV Direct stage-0 frontier, timing and confirmed black screen
- [Matrix-to-Scale result](HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_TEX_COORD_SCALE_FRONTIER.md) — ten texture-matrix returns; preceding Scale frontier and freshness limits
- [IA8 maps 0..7 result](HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_LOAD_TEX_MTX_IMM_FRONTIER.md) — all eight exact texture bindings return before the matrix frontier

- [HARDWARE_RESULTS_2026-09-09.md](HARDWARE_RESULTS_2026-09-09.md) — first recorded hardware evidence
- [HARDWARE_RESULTS_2026-09-10.md](HARDWARE_RESULTS_2026-09-10.md) — hardware-validated runtime bootstrap, memory and translated-product results
- [HARDWARE_DATA_INIT_PASS_2026-09-10.md](HARDWARE_DATA_INIT_PASS_2026-09-10.md) — real-Switch synthetic data-init handoff PASS
- [HARDWARE_RESULTS_2026-09-12.md](HARDWARE_RESULTS_2026-09-12.md) — translated startup blocker progression on real Switch
- [HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md](HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md) — PAL Mario Kart Wii `main()` reached on real Switch
- [HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md](HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md) — ordered post-main progression through MEM2, mutex/thread and GXInit boundaries
- [HARDWARE_RESULTS_2026-09-15_OS_CREATE_THREAD.md](HARDWARE_RESULTS_2026-09-15_OS_CREATE_THREAD.md) — first hardware thread-creation frontier
- [HARDWARE_RESULTS_2026-09-16_OS_INIT_MESSAGE_QUEUE.md](HARDWARE_RESULTS_2026-09-16_OS_INIT_MESSAGE_QUEUE.md) — message-queue frontier
- [HARDWARE_RESULTS_2026-09-16_SELECT_THREAD.md](HARDWARE_RESULTS_2026-09-16_SELECT_THREAD.md) — guest scheduler selection frontier
- [HARDWARE_RESULTS_2026-09-16_OS_LOAD_CONTEXT.md](HARDWARE_RESULTS_2026-09-16_OS_LOAD_CONTEXT.md) — guest context restore frontier
- [HARDWARE_RESULTS_2026-09-16_OS_RECEIVE_MESSAGE.md](HARDWARE_RESULTS_2026-09-16_OS_RECEIVE_MESSAGE.md) — blocking receive frontier
- [HARDWARE_RESULTS_2026-09-16_OS_SLEEP_THREAD.md](HARDWARE_RESULTS_2026-09-16_OS_SLEEP_THREAD.md) — guest wait/sleep frontier
- [HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md](HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md) — HostContext guest-thread continuation proof
- [HARDWARE_RESULTS_2026-09-16_WPAD_INIT.md](HARDWARE_RESULTS_2026-09-16_WPAD_INIT.md) — WPAD initialization frontier
- [HARDWARE_RESULTS_2026-09-16_WPAD_DPD_SENSITIVITY.md](HARDWARE_RESULTS_2026-09-16_WPAD_DPD_SENSITIVITY.md) — WPAD DPD sensitivity frontier
- [HARDWARE_RESULTS_2026-09-16_WPAD_GET_STATUS.md](HARDWARE_RESULTS_2026-09-16_WPAD_GET_STATUS.md) — WPAD status frontier
- [HARDWARE_RESULTS_2026-09-16_WPAD_CONTROL_MOTOR.md](HARDWARE_RESULTS_2026-09-16_WPAD_CONTROL_MOTOR.md) — WPAD motor frontier
- [HARDWARE_RESULTS_2026-09-16_PAD_INIT.md](HARDWARE_RESULTS_2026-09-16_PAD_INIT.md) — PAD initialization frontier
- [HARDWARE_RESULTS_2026-09-16_OS_GET_TIME.md](HARDWARE_RESULTS_2026-09-16_OS_GET_TIME.md) — time-base getter frontier
- [HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md](HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md) — power callback frontier
- [HARDWARE_RESULTS_2026-09-16_SC_GET_PRODUCT_AREA.md](HARDWARE_RESULTS_2026-09-16_SC_GET_PRODUCT_AREA.md) — console product-area frontier
- [HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md](HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md) — scheduler wakeup frontier
- [HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md](HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md) — 37,148-dispatch sustained post-main run and EGG AsyncDisplay attribution
- [HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md](HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md) — 126,563-dispatch run proving active `PostRetraceCallback` / VI retrace progression
- [HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md](HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md) — real-Switch loaderless NVK / `VK_NN_vi_surface` changing-color clear-frame PASS
- [HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md](HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md) — real-Switch shader/pipeline/rasterisation triangle PASS; records the separate SD-report fix
- [HARDWARE_RESULTS_2026-09-18_M3_DAWN_CLEAR.md](HARDWARE_RESULTS_2026-09-18_M3_DAWN_CLEAR.md) — real-Switch Dawn/WebGPU clear/present PASS with 1,507-frame stable loop
- [HARDWARE_RESULTS_2026-09-18_M3_DAWN_TRIANGLE.md](HARDWARE_RESULTS_2026-09-18_M3_DAWN_TRIANGLE.md) — real-Switch WGSL/Dawn graphics-pipeline triangle PASS with clean explicit teardown
- [HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md](HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md) — real-Switch Aurora GX triangle PASS with 563-frame active loop and clean teardown
- [HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md](HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md) — exact pinned `HleFifoWrite` → Aurora GX PASS with 1,435-frame active loop
- [HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md) — FST publication PASS, #185 DVD-read non-reachability, and later priority-6 OSThread `0x90112660` frontier
- [HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md](HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md) — #188 first-fiber register-clobber regression and #189 correction gate
- [HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md) — #189 scheduler recovery PASS and new PAL `OSSendMessage` blocker
- [HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md) — #190 OSSendMessage hardware PASS and new PAL `GXDrawDone` blocker
- [HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md) — #191 GXDrawDone hardware PASS and priority-24 ResourceManager `TaskThread::run` frontier
- [HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md) — first #192 hardware run, TaskThread proof caveat, and PAL `GXSetProjection` frontier
- [HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md) — #193 hardware PASS for TaskThread/projection and new PAL `GXSetViewport` frontier
- [HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md](HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md) — #194 hardware PASS for viewport and new PAL `GXSetScissor` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md) — #195 hardware PASS for scissor and new PAL `GXLoadPosMtxImm` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md) — #196 hardware PASS for `GXLoadPosMtxImm`, TaskThread re-proof, and new PAL `GXSetCurrentMtx` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md) — #197 hardware PASS for `GXSetCurrentMtx` and new PAL `GXClearVtxDesc` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md) — #198 hardware PASS for `GXClearVtxDesc`, TaskThread re-proof, first post-bootstrap GX state FIFO byte, and new PAL `GXSetVtxDesc` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md) — #199 hardware PASS for `GXSetVtxDesc` and new PAL `GXSetVtxAttrFmt` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md) — #200 hardware PASS for `GXSetVtxAttrFmt`, deliberate-abort behavior, and new PAL `GXSetNumChans` frontier
- [HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md](HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md) — #201 hardware PASS for `GXSetNumChans`, TaskThread re-proof, and new PAL `GXSetChanMatColor` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md) — rendered hardware progression beyond `GXSetChanCtrl` and new PAL `GXSetNumTexGens` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md) — rendered hardware progression beyond `GXSetNumTexGens` and new PAL `GXSetNumIndStages` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md) — rendered hardware progression beyond `GXSetNumIndStages`, eleven FIFO writes, and new PAL `GXSetNumTevStages` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md) — rendered hardware progression beyond `GXSetNumTevStages` and new PAL `GXSetTevOp` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md) — rendered hardware progression beyond `GXSetTevOp` and new PAL `GXSetTevOrder` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md) — rendered hardware progression beyond `GXSetTevOrder` and new PAL `GXSetBlendMode` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md) — rendered hardware progression beyond `GXSetBlendMode` and new PAL `GXSetColorUpdate` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_ALPHA_UPDATE_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_ALPHA_UPDATE_FRONTIER.md) — rendered hardware progression beyond `GXSetColorUpdate` and new PAL `GXSetAlphaUpdate` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_Z_MODE_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_Z_MODE_FRONTIER.md) — rendered hardware progression beyond `GXSetAlphaUpdate` and new PAL `GXSetZMode` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_CULL_MODE_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_CULL_MODE_FRONTIER.md) — rendered hardware progression beyond `GXSetZMode` and new PAL `GXSetCullMode` frontier
- [HARDWARE_RESULTS_2026-09-21_GX_BEGIN_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_BEGIN_FRONTIER.md) — rendered hardware progression beyond `GXSetCullMode` to the first PAL `GXBegin` draw-primitive frontier
- [HARDWARE_RESULTS_2026-09-21_GX_SET_COPY_FILTER_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_GX_SET_COPY_FILTER_FRONTIER.md) — first real FIFO work preserved, `endRender` hardware-crossed, and new PAL `GXSetCopyFilter` copy-path frontier
- [HARDWARE_RESULTS_2026-09-21_FIRST_RMCP01_PRESENT_GX_FLUSH_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_FIRST_RMCP01_PRESENT_GX_FLUSH_FRONTIER.md) — first successful game-facing RMCP01 GPU present (`hadWork=1`) and new PAL `GXFlush` frontier
- [HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md](HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md) — `GXFlush` hardware-crossed with 23 successful presents; later TaskThread indirect target equals the worker guest stack pointer and requires job-field diagnostics
- [HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md](HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md) — TaskThread telemetry proves the received “job” aliases the worker stack; next gate is producer-send vs queue-buffer attribution
- [HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md](HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md) — proves `TaskThread::request` sends the valid `mJobs[0]` pointer; that run stops at the exact `OSReceiveMessage` output-slot clobber phase
- [HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md](HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md) — phase trace proves the slot is correct until the wakeup call boundary and defines the minimal fix: no VI retrace polling while guest interrupts are disabled
- [HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md](HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md) — hardware-validates the VI interrupt-mask fix, records the first real `/Boot/Strap/eu/English.szs` read-pass, and moves the frontier to `SELECTTHREAD_IDLE_POLL`
- [HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md](HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md) — identifies default-thread queue `0x804294A4` as `AsyncDisplay + 0x58` and attributes the required idle wake to VI `postVRetrace()`
- [HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md](HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md) — hardware-validates the VI-only AsyncDisplay idle wake, recovers real FIFO/present work, and moves the exact resource frontier to pinned `EGG::Decomp::decodeSZS (0x80218C2C)`
- [HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md](HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md) — proves full `English.szs` SZS expansion and moves the exact rendered frontier to `GXInitTexObj (0x801707F8)`
- [HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md](HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md) — hardware-crosses `GXInitTexObj`, preserves the real FIFO/present path, and moves the frontier to pinned `NAND_IOS_Open (0x801938F8)` with path/mode diagnostics only
- [HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md](HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md) — identifies `/dev/net/kd/request`, mode 0, as the exact IOS_Open request and constrains the next candidate to pinned KD device-handle allocation only
- [HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md](HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md) — hardware-crosses the KD open, captures fd/cmd/in/out for pinned `IOS_Ioctl (0x80194290)` command 2, and defines the exact one-shot Boot-phase `-42` output candidate
- [HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md) — hardware-crosses that first KD command-2 probe and exposes pinned `IOS_Close (0x80193AD8)` with the same fd 2000
- [HARDWARE_RESULTS_2026-09-25_IOS_CLOSE_CROSSED_GX_LOAD_TEX_OBJ_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_IOS_CLOSE_CROSSED_GX_LOAD_TEX_OBJ_FRONTIER.md) — hardware-crosses fd-2000 IOS_Close, records the first `StaticR.rel` read / `RKSystem::run` progress, and moves the frontier to `GXLoadTexObj (0x80170F2C)` diagnostics
- [HARDWARE_RESULTS_2026-09-25_GX_LOAD_TEX_OBJ_DESCRIPTOR_CAPTURED.md](HARDWARE_RESULTS_2026-09-25_GX_LOAD_TEX_OBJ_DESCRIPTOR_CAPTURED.md) — captures the exact first GXLoadTexObj descriptor and defines the strict one-descriptor Aurora bind candidate
- [HARDWARE_RESULTS_2026-09-25_GX_LOAD_TEX_OBJ_CROSSED_TEXCOORDGEN2_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_GX_LOAD_TEX_OBJ_CROSSED_TEXCOORDGEN2_FRONTIER.md) — hardware-crosses that first texture load and moves the exact frontier to GXSetTexCoordGen2
- [HARDWARE_RESULTS_2026-09-25_TEXCOORDGEN2_CROSSED_STRAP_CHECK_INPUT_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_TEXCOORDGEN2_CROSSED_STRAP_CHECK_INPUT_FRONTIER.md) — hardware-crosses GXSetTexCoordGen2, records sustained 60-frame rendering, and moves the exact frontier to StrapScene::CheckInput
- [HARDWARE_RESULTS_2026-09-25_STRAP_CHECK_INPUT_CROSSED_STATICR_REL_PROLOG_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_STRAP_CHECK_INPUT_CROSSED_STATICR_REL_PROLOG_FRONTIER.md) — hardware-crosses StrapScene::CheckInput, records 61 successful presents / 0 failures, and attributes the first StaticR native-wrapper frontier to RelProlog at 0x8055531C
- [HARDWARE_RESULTS_2026-09-25_STATICR_REL_PROLOG_CROSSED_OS_DETACH_THREAD_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_STATICR_REL_PROLOG_CROSSED_OS_DETACH_THREAD_FRONTIER.md) — hardware-crosses StaticR RelProlog and moves the frontier to OSDetachThread diagnostics
- [HARDWARE_RESULTS_2026-09-25_OS_DETACH_THREAD_LIVE_PATH.md](HARDWARE_RESULTS_2026-09-25_OS_DETACH_THREAD_LIVE_PATH.md) — captures the exact WAITING/already-detached/empty-join TaskThread state and defines the strict first OSDetachThread candidate
- [HARDWARE_RESULTS_2026-09-25_OS_DETACH_THREAD_CROSSED_OS_CANCEL_THREAD_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_OS_DETACH_THREAD_CROSSED_OS_CANCEL_THREAD_FRONTIER.md) — hardware-crosses OSDetachThread and moves the exact frontier to OSCancelThread diagnostics
- [HARDWARE_RESULTS_2026-09-25_OS_CANCEL_THREAD_LIVE_PATH.md](HARDWARE_RESULTS_2026-09-25_OS_CANCEL_THREAD_LIVE_PATH.md) — captures the complete WAITING/detached/singleton-queue OSCancelThread state and defines the strict first cancellation candidate
- [HARDWARE_RESULTS_2026-09-25_OS_CANCEL_THREAD_CROSSED_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-25_OS_CANCEL_THREAD_CROSSED_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses OSCancelThread, records Home Button/UI resource progress and moves the exact frontier to GXInitTexObjLOD diagnostics
- [HARDWARE_RESULTS_2026-09-25_GX_INIT_TEX_OBJ_LOD_TUPLE_CAPTURED.md](HARDWARE_RESULTS_2026-09-25_GX_INIT_TEX_OBJ_LOD_TUPLE_CAPTURED.md) — captures the complete first Home Button/UI LOD tuple and defines the strict one-tuple GXInitTexObjLOD candidate
- [HARDWARE_RESULTS_2026-09-26_GX_INIT_TEX_OBJ_WRAP_MODE_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_GX_INIT_TEX_OBJ_WRAP_MODE_FRONTIER.md) — crosses the first LOD tuple and exposes the first exact wrap tuple
- [HARDWARE_RESULTS_2026-09-26_SECOND_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_SECOND_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — crosses the first wrap and captures the second exact LOD descriptor
- [HARDWARE_RESULTS_2026-09-26_SECOND_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_SECOND_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — crosses the second LOD descriptor and captures its exact wrap tuple
- [HARDWARE_RESULTS_2026-09-26_THIRD_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_THIRD_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — crosses the second LOD descriptor on the alternate ordering and captures the third LOD descriptor
- [HARDWARE_RESULTS_2026-09-26_THIRD_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_THIRD_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — crosses the third LOD descriptor and captures the third exact wrap tuple
- [HARDWARE_RESULTS_2026-09-26_SECOND_KD_CLOSE_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_SECOND_KD_CLOSE_FRONTIER.md) — records the second KD request close frontier
- [HARDWARE_RESULTS_2026-09-26_THIRD_KD_USER_ID_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_THIRD_KD_USER_ID_FRONTIER.md) — records fd-2002 command-0x0F generated-user-id semantics
- [HARDWARE_RESULTS_2026-09-26_THIRD_KD_CLOSE_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_THIRD_KD_CLOSE_FRONTIER.md) — crosses the third KD request command and captures fd-2002 close
- [HARDWARE_RESULTS_2026-09-26_FOURTH_KD_RESUME_FRONTIER.md](HARDWARE_RESULTS_2026-09-26_FOURTH_KD_RESUME_FRONTIER.md) — crosses fd-2002 close and captures fd-2003 command-3 resume
- [HARDWARE_RESULTS_2026-09-27_FOURTH_KD_CLOSE_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_FOURTH_KD_CLOSE_FRONTIER.md) — crosses fd-2003 command-3 and captures the exact fd-2003 IOS_Close frontier
- [HARDWARE_RESULTS_2026-09-27_AI_INIT_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_AI_INIT_FRONTIER.md) — hardware-crosses fd-2003 IOS_Close and captures PAL AIInit (0x801240B0) as the next exact frontier
- [HARDWARE_RESULTS_2026-09-27_AX_OUT_INIT_DSP_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_AX_OUT_INIT_DSP_FRONTIER.md) — hardware-crosses PAL AIInit and captures __AXOutInitDSP (0x801269BC) as the next exact audio frontier
- [HARDWARE_RESULTS_2026-09-27_AI_REGISTER_DMA_CALLBACK_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_AI_REGISTER_DMA_CALLBACK_FRONTIER.md) — hardware-crosses __AXOutInitDSP and captures AIRegisterDMACallback (0x80123F88) as the next exact audio frontier
- [HARDWARE_RESULTS_2026-09-27_AI_INIT_DMA_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_AI_INIT_DMA_FRONTIER.md) — hardware-crosses AIRegisterDMACallback and captures AIInitDMA (0x80123FCC) as the next exact audio frontier
- [HARDWARE_RESULTS_2026-09-27_AI_START_DMA_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_AI_START_DMA_FRONTIER.md) — hardware-crosses AIInitDMA and captures AIStartDMA (0x80124048) as the next exact audio frontier
- [HARDWARE_RESULTS_2026-09-27_FOURTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_FOURTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses AIStartDMA and the third wrap tuple, then captures the fourth exact GXInitTexObjLOD descriptor on 0x9018E480
- [HARDWARE_RESULTS_2026-09-27_OS_SET_PERIODIC_ALARM_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_OS_SET_PERIODIC_ALARM_FRONTIER.md) — hardware-crosses the fourth GXInitTexObjLOD descriptor and captures OSSetPeriodicAlarm (0x801A08E0)
- [HARDWARE_RESULTS_2026-09-27_SOUND_PLAYER_SET_VOLUME_FRONTIER.md](HARDWARE_RESULTS_2026-09-27_SOUND_PLAYER_SET_VOLUME_FRONTIER.md) — hardware-crosses OSSetPeriodicAlarm, records real revo_kart.brsar reads, and captures nw4r::snd::SoundPlayer::SetVolume (0x800A35E0)
- [HARDWARE_RESULTS_2026-09-28_FIFTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_FIFTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses SoundPlayer::SetVolume and captures the fifth exact GXInitTexObjLOD descriptor on 0x908FA4E0
- [HARDWARE_RESULTS_2026-09-28_FOURTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_FOURTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — hardware-crosses the fifth GXInitTexObjLOD descriptor and captures the fourth exact GXInitTexObjWrapMode tuple on 0x908FA4E0
- [HARDWARE_RESULTS_2026-09-28_SIXTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_SIXTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses the fourth GXInitTexObjWrapMode tuple on 0x908FA4E0 and captures the sixth exact GXInitTexObjLOD descriptor on 0x908FA5C0
- [HARDWARE_RESULTS_2026-09-28_SEVENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_SEVENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses the sixth exact GXInitTexObjLOD descriptor on 0x908FA5C0 and captures the seventh exact descriptor on 0x907938A0
- [HARDWARE_RESULTS_2026-09-28_EIGHTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_EIGHTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — moves durably beyond the seventh exact GXInitTexObjLOD descriptor on 0x907938A0 and captures the eighth exact descriptor on 0x908FA820
- [HARDWARE_RESULTS_2026-09-28_FIFTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_FIFTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — records the seventh GXInitTexObjLOD on 0x907938A0 as lod-pass and captures its fifth exact GXInitTexObjWrapMode tuple
- [HARDWARE_RESULTS_2026-09-28_SIXTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-28_SIXTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — records the sixth GXInitTexObjLOD on 0x908FA5C0 as lod-pass and captures its sixth exact GXInitTexObjWrapMode tuple
- [HARDWARE_RESULTS_2026-09-29_NINTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-29_NINTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses the fifth GXInitTexObjWrapMode tuple on 0x907938A0 and captures the ninth exact GXInitTexObjLOD descriptor on 0x90793BE0
- [HARDWARE_RESULTS_2026-09-29_TENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-29_TENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — captures the tenth exact GXInitTexObjLOD descriptor on 0x909019C0 at the strongest translated-dispatch frontier so far
- [HARDWARE_RESULTS_2026-09-29_SEVENTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-29_SEVENTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — hardware-crosses the eighth GXInitTexObjLOD descriptor on 0x908FA820 and captures its seventh exact GXInitTexObjWrapMode tuple
- [HARDWARE_RESULTS_2026-09-29_ELEVENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-29_ELEVENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses the seventh GXInitTexObjWrapMode tuple on 0x908FA820 and captures the first format-2 eleventh exact GXInitTexObjLOD descriptor on 0x908FA840
- [HARDWARE_RESULTS_2026-09-29_NINTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-09-29_NINTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — hardware-crosses the eleventh GXInitTexObjLOD descriptor on 0x908FA840 and captures its ninth exact GXInitTexObjWrapMode tuple
- [HARDWARE_RESULTS_2026-09-29_TWELFTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-29_TWELFTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — captures a second exact, format-2 GXInitTexObjLOD descriptor on the already-known 0x9018E480 and attributes the sequence to FUN_801813e0 at function level
- [HARDWARE_RESULTS_2026-09-30_THIRTEENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md](HARDWARE_RESULTS_2026-09-30_THIRTEENTH_GX_INIT_TEX_OBJ_LOD_FRONTIER.md) — hardware-crosses the ninth GXInitTexObjWrapMode tuple on 0x908FA840 and captures the thirteenth exact GXInitTexObjLOD descriptor on 0x908FAE00
- [HARDWARE_RESULTS_2026-10-01_TENTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-10-01_TENTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — records the hardware-proven format-0 LOD on 0x9018E480 and captures its first exact clamp/clamp wrap tuple
- [HARDWARE_RESULTS_2026-10-01_ELEVENTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md](HARDWARE_RESULTS_2026-10-01_ELEVENTH_GX_INIT_TEX_OBJ_WRAP_FRONTIER.md) — hardware-crosses the thirteenth GXInitTexObjLOD descriptor on 0x908FAE00 and captures its eleventh exact GXInitTexObjWrapMode tuple
- [HARDWARE_RESULTS_2026-09-21_FIRST_RMCP01_FIFO_WORK_END_RENDER_FRONTIER.md](HARDWARE_RESULTS_2026-09-21_FIRST_RMCP01_FIFO_WORK_END_RENDER_FRONTIER.md) — `GXBegin` hardware-crossed, first real RMCP01 FIFO/Aurora render work, and new `EGG::AsyncDisplay::endRender` frontier

## Blocker notes

- [TEV color/table batch](GX_TEV_COLOR_BATCH_2026-10-03.md) — KColor pointer fix and pre-ported Color/SwapModeTable; host/remote/private-build gates pass, console return pending

- [TEV scalar batch](GX_TEV_SCALAR_BATCH_2026-10-03.md) — six setters, legal SDK domains, completed gates and default-tuple hardware acceptance
- [Coordinate batch hardware scope](GX_TEX_COORD_BATCH_2026-10-03.md) — eight exact triples accepted; enabled branches remain host-only
- [Coordinate neighbor audit](GX_TEX_COORD_NEIGHBORS_2026-10-02.md) — pinned wrapper/Aurora semantics and static caller
- [TEV neighbor audit](GX_TEV_NEIGHBORS_2026-10-03.md) — pinned contracts and static look-ahead; the separate scalar result accepts only the executed default loop

- [fast-track-blockers/](fast-track-blockers/) — blocker-specific mapping, pinned semantics and fix notes

For the current project status, use [README.md](../README.md),
[ROADMAP.md](../ROADMAP.md),
[FAST_TRACK_VALIDATION_POLICY.md](FAST_TRACK_VALIDATION_POLICY.md),
[M3_RMCP01_RENDERED_FAST_TRACK.md](M3_RMCP01_RENDERED_FAST_TRACK.md), and issue #117.

Current accepted state as of 2026-10-03:

- real RMCP01 FIFO work and repeated GPU presents are hardware-proven;
- the latest accepted pre-matrix snapshot records 1556 FIFO writes and 99
  successful presents / 0 failures; it does not measure later matrix emissions;
- the exact IA8 descriptor loads on maps 0..7 and ten type-0 texture-matrix
  calls remain crossed;
- all eight Gen2(c,1,4,60,0,125), disabled Scale(c,0,0,0) and disabled
  Bias(c,0,0) triples are accepted for c=0..7; enabled branches remain host-only;
- all six scalar TEV setters return on default tuples for stages 0..15:
  96 new calls and 16 existing Order calls;
- the durable frontier is KColor `0x80171ED4`, ID 0, pointer `0x80398FCC`,
  LR `0x80240F98`, dispatch 605056 / 98.265 seconds;
- the user confirmed black screens for coordinate NRO `64ba8377...` and
  audit NRO `7ecbc8a9...`, and black output plus an error at exit for TEV
  NRO `cc88a78c...`; no recognizable Mario Kart Wii image is proven;
- audit NRO `7ecbc8a9...` is accepted for normal-path non-regression only;
  SIZE_MAX, Present(false), teardown/shutdown error paths were not exercised;
- the TEV batch passed local gates, all five GitHub workflows on code
  `e76e8f38`, its private build and bounded console progression;
  alternate tuples remain host-only, KColor remains unreturned.

See [the latest accepted hardware report](HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md),
[the coordinate candidate](GX_TEX_COORD_BATCH_2026-10-03.md), and
[the static TEV audit](GX_TEV_NEIGHBORS_2026-10-03.md).

Dated hardware-result files are historical evidence and intentionally retain
the frontier wording that was correct when each run was captured.

- [Viewport/depth returned; Mii I4 frontier](HARDWARE_RESULTS_2026-10-07_VIEWPORT_MII_I4_LOAD_FRONTIER.md) — verified 37-report Netloader run and observed native returns.
- [Bounded Mii I4 load](GX_MII_I4_LOAD_2026-10-07.md) — exact descriptor, complete 1,024-byte physical MEM2 range and guarded native binding.

- [First I4 load returned; second-object frontier](HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_LOAD_FRONTIER.md) — verified 37-report run and checked-caller return proof.
- [Second captured Mii I4 object](GX_MII_I4_SECOND_LOAD_2026-10-07.md) — exact two-address guard and separate host objects sharing one checked payload.
