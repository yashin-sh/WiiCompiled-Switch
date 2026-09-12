#include "context_probe.hpp"
#include "guest_flat_api_probe.hpp"
#include "horizon_guest_flat.hpp"
#include "horizon_runtime_services.hpp"
#include "memory_init_probe.hpp"
#include "runtime_bootstrap.hpp"
#include "switch_platform.hpp"
#include "vm_probe.hpp"

#include <switch.h>
#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

int main(int, char**) {
    mkw_switch_set_fast_track_stage("MAIN_PLATFORM_INIT");
    auto info = mkw::switch_platform::initialize();
    mkw::switch_platform::present_bootstrap_screen(info);

#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
    // The VM/context/GuestFlat smoke probes were hardware-validated earlier in
    // the port. Re-running them on every blocker adds startup noise and can hide
    // failures in the real boot path. Local fast-track builds now go straight
    // to the runtime bootstrap; public/default builds keep the probes intact.
    mkw_switch_set_fast_track_stage("FAST_TRACK_SKIP_VALIDATED_SMOKES");
    std::printf("Local fast-track: skipping validated VM/context/GuestFlat smoke probes.\n");
    consoleUpdate(nullptr);
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
    mkw::runtime_bootstrap::print(runtime_result);
    if (!mkw::runtime_bootstrap::write_report(runtime_result)) {
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

    mkw_switch_set_fast_track_stage("IDLE_AFTER_BOOTSTRAP");
    while (!mkw::switch_platform::should_exit()) {
        mkw::horizon_runtime_services::sleep_for_ns(16'000'000); // bootstrap only
    }

    mkw_switch_set_fast_track_stage("SHUTDOWN");
    mkw::runtime_bootstrap::stop();
    mkw::switch_platform::shutdown();
    return 0;
}
