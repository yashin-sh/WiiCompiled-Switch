#include "switch_platform.hpp"

#include "horizon_runtime_services.hpp"

#include <switch.h>
#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace mkw::switch_platform {
namespace {
bool g_console_initialized = false;
bool g_romfs_initialized = false;
}

BootstrapInfo initialize() {
#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
    // The blocker-driven local fast-track is intentionally headless. libnx's
    // default PrintConsole renderer creates a framebuffer, which initializes
    // nvdrv and allocates/zeros an 8 MiB TransferMemory block. That display path
    // is unrelated to translated startup and has produced a pre-guest host Data
    // Abort on hardware. Keep the fast-track focused on SD diagnostics and the
    // translated runtime until a real GX backend replaces the current FIFO sink.
    mkw_switch_set_fast_track_stage("PLATFORM_CONSOLE_SKIPPED_FAST_TRACK");
#else
    mkw_switch_set_fast_track_stage("PLATFORM_CONSOLE_INIT");
    consoleInit(nullptr);
    g_console_initialized = true;
    mkw_switch_set_fast_track_stage("PLATFORM_CONSOLE_READY");
#endif

    mkw_switch_set_fast_track_stage("PLATFORM_SERVICES_INIT");
    const auto services = horizon_runtime_services::initialize();
    mkw_switch_set_fast_track_stage("PLATFORM_SERVICES_READY");

    mkw_switch_set_fast_track_stage("PLATFORM_ROMFS_INIT");
    const Result romfs_rc = romfsInit();
    g_romfs_initialized = R_SUCCEEDED(romfs_rc);
    mkw_switch_set_fast_track_stage("PLATFORM_READY");

    return BootstrapInfo{
        .sd_mounted = services.filesystem_ready,
        .ticks = horizon_runtime_services::monotonic_ticks(),
    };
}

void shutdown() {
    if (g_romfs_initialized) {
        romfsExit();
        g_romfs_initialized = false;
    }
    horizon_runtime_services::shutdown();
    if (g_console_initialized) {
        consoleExit(nullptr);
        g_console_initialized = false;
    }
}

bool should_exit() {
    return horizon_runtime_services::should_exit();
}

void present_bootstrap_screen(const BootstrapInfo& info) {
#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
    // No PrintConsole/NV framebuffer in the local fast-track. All actionable
    // diagnostics are persisted to SD by the fast-track crash/blocker path.
    (void)info;
    return;
#else
    const auto services = horizon_runtime_services::status();

    std::printf("WiiCompiled-Switch\n");
    std::printf("==================\n\n");
    std::printf("M2 Horizon runtime bootstrap\n");
    std::printf("AArch64 / Horizon homebrew runtime is alive.\n\n");
    std::printf("Initial system tick: %llu\n", static_cast<unsigned long long>(info.ticks));
    std::printf("SD filesystem: %s\n", info.sd_mounted ? "available" : "unavailable");
    std::printf("libnx HID: %s\n", services.input_ready ? "ready" : "unavailable");
    std::printf("SDL critical path: none\n");
    std::printf("Audio: stubbed; graphics: stubbed\n");
    std::printf("RomFS probe: %s\n\n", g_romfs_initialized ? "initialized" : "not present (expected for now)");
    std::printf("Press + to exit.\n");
    consoleUpdate(nullptr);
#endif
}

} // namespace mkw::switch_platform
