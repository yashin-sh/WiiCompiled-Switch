#include "switch_platform.hpp"

#include <switch.h>
#include <cstdio>

namespace mkw::switch_platform {
namespace {
PadState g_pad{};
bool g_console_initialized = false;
bool g_romfs_initialized = false;
}

BootstrapInfo initialize() {
    consoleInit(nullptr);
    g_console_initialized = true;

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);

    const Result romfs_rc = romfsInit();
    g_romfs_initialized = R_SUCCEEDED(romfs_rc);

    return BootstrapInfo{
        .sd_mounted = true,
        .ticks = armGetSystemTick(),
    };
}

void shutdown() {
    if (g_romfs_initialized) {
        romfsExit();
        g_romfs_initialized = false;
    }
    if (g_console_initialized) {
        consoleExit(nullptr);
        g_console_initialized = false;
    }
}

bool should_exit() {
    padUpdate(&g_pad);
    const u64 down = padGetButtonsDown(&g_pad);
    return !appletMainLoop() || (down & HidNpadButton_Plus);
}

void present_bootstrap_screen(const BootstrapInfo& info) {
    std::printf("WiiCompiled-Switch\n");
    std::printf("==================\n\n");
    std::printf("Milestone 0: libnx bootstrap\n");
    std::printf("AArch64 / Horizon homebrew runtime is alive.\n\n");
    std::printf("Initial system tick: %llu\n", static_cast<unsigned long long>(info.ticks));
    std::printf("SD filesystem: %s\n", info.sd_mounted ? "available" : "unavailable");
    std::printf("RomFS probe: %s\n\n", g_romfs_initialized ? "initialized" : "not present (expected for now)");
    std::printf("Press + to exit.\n");
    consoleUpdate(nullptr);
}

} // namespace mkw::switch_platform
