#include "switch_platform.hpp"

#include "horizon_runtime_services.hpp"

#include <switch.h>
#include <cstdio>

namespace mkw::switch_platform {
namespace {
bool g_console_initialized = false;
bool g_romfs_initialized = false;
}

BootstrapInfo initialize() {
    consoleInit(nullptr);
    g_console_initialized = true;

    const auto services = horizon_runtime_services::initialize();

    const Result romfs_rc = romfsInit();
    g_romfs_initialized = R_SUCCEEDED(romfs_rc);

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
}

} // namespace mkw::switch_platform
