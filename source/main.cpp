#include "context_probe.hpp"
#include "guest_flat_api_probe.hpp"
#include "horizon_guest_flat.hpp"
#include "horizon_runtime_services.hpp"
#include "memory_init_probe.hpp"
#include "runtime_bootstrap.hpp"
#include "rendered_fast_track_graphics.hpp"
#include "switch_platform.hpp"
#include "vm_probe.hpp"
#include "mkw_debug_server.hpp"

#include <switch.h>
#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;
extern "C" bool mkw_switch_stop_liveness_watchdog() noexcept;

namespace {
void JoinDiagnosticWorkers() noexcept {
    // Failed SDK joins retain their handle ownership for a retry. No loader
    // return is safe until both independent workers have really terminated.
    while (!mkw::debug::stop() || !mkw_switch_stop_liveness_watchdog()) {
        mkw_switch_set_fast_track_stage("SHUTDOWN_WORKER_JOIN_FAILED");
        mkw::horizon_runtime_services::sleep_for_ns(50'000'000);
    }
}
} // namespace

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
struct CpuContext;
extern "C" void mkw_switch_hle_gx_draw_done(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_projection(CpuContext* ctx);
extern "C" void mkw_switch_hle_gx_set_viewport(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_scissor(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_load_pos_mtx_imm(CpuContext* ctx);
extern "C" void mkw_switch_hle_gx_set_current_mtx(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_clear_vtx_desc(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_vtx_desc(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_vtx_attr_fmt(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_tex_gens(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_ind_stages(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_tev_stages(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_op(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_order(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_blend_mode(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_color_update(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_alpha_update(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_z_mode(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_cull_mode(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_begin(CpuContext* ctx);
extern "C" void mkw_switch_hle_gx_set_num_chans(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_mat_color(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_ctrl(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_set_copy_filter(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_gx_flush(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_os_create_thread(CpuContext* ctx) noexcept;
extern "C" void mkw_switch_hle_select_thread(CpuContext* ctx);
extern "C" void mkw_switch_hle_os_send_message(CpuContext* ctx);
extern "C" void mkw_switch_hle_os_receive_message(CpuContext* ctx);
extern "C" void mkw_switch_hle_os_sleep_thread(CpuContext* ctx);
#endif

int main(int argc, char** argv) {
    mkw::debug::set_launch_path(argc > 0 && argv ? argv[0] : nullptr);
#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
    // The synthetic fast-track NRO is a link probe, not a scheduler/runtime
    // test. Keep the real bridges reachable through --gc-sections so unresolved
    // dependencies that would break the local devkitA64 fast-track are caught
    // by public CI without executing guest scheduler semantics against fake state.
    volatile auto gx_draw_done_link_anchor = &mkw_switch_hle_gx_draw_done;
    volatile auto gx_set_projection_link_anchor = &mkw_switch_hle_gx_set_projection;
    volatile auto gx_set_viewport_link_anchor = &mkw_switch_hle_gx_set_viewport;
    volatile auto gx_set_scissor_link_anchor = &mkw_switch_hle_gx_set_scissor;
    volatile auto gx_load_pos_mtx_imm_link_anchor = &mkw_switch_hle_gx_load_pos_mtx_imm;
    volatile auto gx_set_current_mtx_link_anchor = &mkw_switch_hle_gx_set_current_mtx;
    volatile auto gx_clear_vtx_desc_link_anchor = &mkw_switch_hle_gx_clear_vtx_desc;
    volatile auto gx_set_vtx_desc_link_anchor = &mkw_switch_hle_gx_set_vtx_desc;
    volatile auto gx_set_vtx_attr_fmt_link_anchor = &mkw_switch_hle_gx_set_vtx_attr_fmt;
    volatile auto gx_set_num_tex_gens_link_anchor = &mkw_switch_hle_gx_set_num_tex_gens;
    volatile auto gx_set_num_ind_stages_link_anchor = &mkw_switch_hle_gx_set_num_ind_stages;
    volatile auto gx_set_num_tev_stages_link_anchor = &mkw_switch_hle_gx_set_num_tev_stages;
    volatile auto gx_set_tev_op_link_anchor = &mkw_switch_hle_gx_set_tev_op;
    volatile auto gx_set_tev_order_link_anchor = &mkw_switch_hle_gx_set_tev_order;
    volatile auto gx_set_blend_mode_link_anchor = &mkw_switch_hle_gx_set_blend_mode;
    volatile auto gx_set_color_update_link_anchor = &mkw_switch_hle_gx_set_color_update;
    volatile auto gx_set_alpha_update_link_anchor = &mkw_switch_hle_gx_set_alpha_update;
    volatile auto gx_set_z_mode_link_anchor = &mkw_switch_hle_gx_set_z_mode;
    volatile auto gx_set_cull_mode_link_anchor = &mkw_switch_hle_gx_set_cull_mode;
    volatile auto gx_begin_link_anchor = &mkw_switch_hle_gx_begin;
    volatile auto gx_set_num_chans_link_anchor = &mkw_switch_hle_gx_set_num_chans;
    volatile auto gx_set_chan_mat_color_link_anchor = &mkw_switch_hle_gx_set_chan_mat_color;
    volatile auto gx_set_chan_ctrl_link_anchor = &mkw_switch_hle_gx_set_chan_ctrl;
    volatile auto gx_set_copy_filter_link_anchor = &mkw_switch_hle_gx_set_copy_filter;
    volatile auto gx_flush_link_anchor = &mkw_switch_hle_gx_flush;
    volatile auto os_create_thread_link_anchor = &mkw_switch_hle_os_create_thread;
    volatile auto select_thread_link_anchor = &mkw_switch_hle_select_thread;
    volatile auto os_send_message_link_anchor = &mkw_switch_hle_os_send_message;
    volatile auto os_receive_message_link_anchor = &mkw_switch_hle_os_receive_message;
    volatile auto os_sleep_thread_link_anchor = &mkw_switch_hle_os_sleep_thread;
    (void)gx_draw_done_link_anchor;
    (void)gx_set_projection_link_anchor;
    (void)gx_set_viewport_link_anchor;
    (void)gx_set_scissor_link_anchor;
    (void)gx_load_pos_mtx_imm_link_anchor;
    (void)gx_set_current_mtx_link_anchor;
    (void)gx_clear_vtx_desc_link_anchor;
    (void)gx_set_vtx_desc_link_anchor;
    (void)gx_set_vtx_attr_fmt_link_anchor;
    (void)gx_set_num_tex_gens_link_anchor;
    (void)gx_set_num_ind_stages_link_anchor;
    (void)gx_set_num_tev_stages_link_anchor;
    (void)gx_set_tev_op_link_anchor;
    (void)gx_set_tev_order_link_anchor;
    (void)gx_set_blend_mode_link_anchor;
    (void)gx_set_color_update_link_anchor;
    (void)gx_set_alpha_update_link_anchor;
    (void)gx_set_z_mode_link_anchor;
    (void)gx_set_cull_mode_link_anchor;
    (void)gx_begin_link_anchor;
    (void)gx_set_num_chans_link_anchor;
    (void)gx_set_chan_mat_color_link_anchor;
    (void)gx_set_chan_ctrl_link_anchor;
    (void)gx_set_copy_filter_link_anchor;
    (void)gx_flush_link_anchor;
    (void)os_create_thread_link_anchor;
    (void)select_thread_link_anchor;
    (void)os_send_message_link_anchor;
    (void)os_receive_message_link_anchor;
    (void)os_sleep_thread_link_anchor;
#endif

    mkw_switch_set_fast_track_stage("MAIN_PLATFORM_INIT");
    auto info = mkw::switch_platform::initialize();
    mkw::switch_platform::present_bootstrap_screen(info);

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    mkw_switch_set_fast_track_stage("RENDERER_INIT");
    if (!mkw_switch_renderer_initialize()) {
        mkw_switch_set_fast_track_stage("RENDERER_INIT_FAILED");
        JoinDiagnosticWorkers();
        mkw::switch_platform::shutdown();
        return 2;
    }
    mkw_switch_set_fast_track_stage("RENDERER_READY");
#endif

#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
    // The VM/context/GuestFlat smoke probes were hardware-validated earlier in
    // the port. Re-running them on every blocker adds startup noise and can hide
    // failures in the real boot path. The local fast-track is also intentionally
    // headless, so do not depend on stdout/PrintConsole here.
    mkw_switch_set_fast_track_stage("FAST_TRACK_SKIP_VALIDATED_SMOKES");
#else
    mkw_switch_set_fast_track_stage("VM_PROBE");
    const auto vm_result = mkw::vm_probe::run();
    mkw::vm_probe::print(vm_result);
    if (!mkw::vm_probe::write_report(vm_result)) {
        std::printf("WARNING: could not write vm-probe.txt to the app folder.\n");
        consoleUpdate(nullptr);
    }

    mkw_switch_set_fast_track_stage("CONTEXT_PROBE");
    const auto context_result = mkw::context_probe::run();
    mkw::context_probe::print(context_result);
    if (!mkw::context_probe::append_report(context_result)) {
        std::printf("WARNING: could not append context results to vm-probe.txt.\n");
        consoleUpdate(nullptr);
    }

    mkw_switch_set_fast_track_stage("GUESTFLAT_SMOKE");
    const auto guest_flat_result = mkw::horizon_guest_flat::run_smoke_test();
    mkw::horizon_guest_flat::print_smoke_result(guest_flat_result);
    if (!mkw::horizon_guest_flat::append_smoke_report(guest_flat_result)) {
        std::printf("WARNING: could not append GuestFlat results to vm-probe.txt.\n");
        consoleUpdate(nullptr);
    }

    mkw_switch_set_fast_track_stage("GUESTFLAT_API_PROBE");
    const auto guest_flat_api_result = mkw::guest_flat_api_probe::run();
    mkw::guest_flat_api_probe::print(guest_flat_api_result);
    if (!mkw::guest_flat_api_probe::append_report(guest_flat_api_result)) {
        std::printf("WARNING: could not append GuestFlat API results to vm-probe.txt.\n");
        consoleUpdate(nullptr);
    }

    mkw_switch_set_fast_track_stage("MEMORY_INIT_PROBE");
    const auto memory_init_result = mkw::memory_init_probe::run();
    mkw::memory_init_probe::print(memory_init_result);
    if (!mkw::memory_init_probe::append_report(memory_init_result)) {
        std::printf("WARNING: could not append Memory::Init results to vm-probe.txt.\n");
        consoleUpdate(nullptr);
    }
#endif

    mkw_switch_set_fast_track_stage("RUNTIME_BOOTSTRAP_START");
    const auto runtime_result = mkw::runtime_bootstrap::start();
    mkw_switch_set_fast_track_stage("RUNTIME_BOOTSTRAP_RETURNED");

#if !(defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK)
    mkw::runtime_bootstrap::print(runtime_result);
#endif
    const bool report_written = mkw::runtime_bootstrap::write_report(runtime_result);

#if !(defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK)
    if (!report_written) {
        std::printf("WARNING: could not write runtime-bootstrap.txt.\n");
        consoleUpdate(nullptr);
    }

    switch (runtime_result.stop_point) {
    case mkw::runtime_bootstrap::StopPoint::WaitingForTranslatedProduct:
        std::printf("Runtime core reached WAITING_FOR_TRANSLATED_PRODUCT.\n");
        break;
    case mkw::runtime_bootstrap::StopPoint::TranslatedProductLinked:
        std::printf("Translated product is linked; execution handoff is not enabled yet.\n");
        break;
    case mkw::runtime_bootstrap::StopPoint::Failed:
    default:
        std::printf("Runtime core stopped before the translated-product boundary.\n");
        break;
    }
    std::printf("Press + to exit.\n");
    consoleUpdate(nullptr);
#else
    (void)report_written;
#endif

    mkw_switch_set_fast_track_stage("IDLE_AFTER_BOOTSTRAP");
    while (true) {
        if (mkw::debug::reload_requested()) {
            // The execution provider must have unwound every guest stack;
            // envSetNextLoad schedules the loader return, it does not exit.
            if (!runtime_result.translated_execution_cancelled || runtime_result.guest_quiescent) {
                if (mkw::runtime_bootstrap::quiesce_guest()) {
                    if (mkw::debug::prepare_reload_on_owner())
                        break;
                } else {
                    mkw::debug::set_reload_owner_available(false);
                    mkw_switch_set_fast_track_stage("RELOAD_GUEST_DRAIN_FAILED");
                }
            } else {
                mkw::debug::set_reload_owner_available(false);
            }
        }
        if (mkw::switch_platform::should_exit())
            break;
        mkw::horizon_runtime_services::sleep_for_ns(16'000'000);
    }

    if (!mkw::runtime_bootstrap::quiesce_guest()) {
        mkw::debug::set_reload_owner_available(false);
        mkw_switch_set_fast_track_stage("SHUTDOWN_GUEST_DRAIN_FAILED");
        // Do not return to a loader or destroy backing while guards are live.
        for (;;)
            mkw::horizon_runtime_services::sleep_for_ns(50'000'000);
    }

    mkw_switch_set_fast_track_stage("SHUTDOWN");
    // No background diagnostics may survive a normal hbloader return.
    JoinDiagnosticWorkers();
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    if (!mkw_switch_renderer_shutdown()) {
        mkw_switch_set_fast_track_stage("RELOAD_RENDERER_DRAIN_FAILED");
        // Next-load buffers take effect only after normal main return.
        for (;;)
            mkw::horizon_runtime_services::sleep_for_ns(50'000'000);
    }
#endif
    if (!mkw::runtime_bootstrap::stop()) {
        // Preserve memory and suspended C++ stacks rather than freeing backing
        // under live CPU/FPR guards. A failed drain cannot return to hbloader.
        for (;;)
            mkw::horizon_runtime_services::sleep_for_ns(50'000'000);
    }
    mkw::switch_platform::shutdown();
    return 0;
}
