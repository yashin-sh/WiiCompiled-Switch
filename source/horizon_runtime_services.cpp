#include "horizon_runtime_services.hpp"

#include <platform/host_platform.h>
#include <switch.h>

#include <filesystem>

namespace mkw::horizon_runtime_services {
namespace {

PadState g_pad{};
bool g_initialized = false;
Status g_status{};

} // namespace

std::filesystem::path application_root() {
    return RuntimePlatform::ApplicationDataDirectory("WiiCompiled-Switch");
}

std::filesystem::path user_game_data_root() {
    // This directory is intentionally never populated by the project or CI.
    // A user may place locally produced/game-owned inputs here when later
    // milestones define the exact runtime layout.
    return application_root() / "game-data";
}

Status initialize() {
    if (g_initialized) {
        return g_status;
    }

    // Make the unmodified upstream RuntimePlatform POSIX fallback resolve to a
    // stable SD-card root. It uses current_path()/applicationName on non-Win32,
    // non-Apple hosts, which is sufficient for this first Horizon slice.
    std::error_code cwd_ec;
    std::filesystem::current_path("sdmc:/switch", cwd_ec);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);

    std::error_code dir_ec;
    std::filesystem::create_directories(application_root() / "Logs", dir_ec);

    g_status.lifecycle_ready = true;
    g_status.filesystem_ready = !cwd_ec && !dir_ec;
    g_status.timing_ready = armGetSystemTickFreq() != 0;
    g_status.input_ready = true;
    g_status.audio = BackendState::Stubbed;
    g_status.graphics = BackendState::Stubbed;
    g_initialized = true;
    return g_status;
}

Status status() noexcept {
    return g_status;
}

void shutdown() noexcept {
    g_initialized = false;
    g_status = {};
}

InputState poll_input() {
    InputState result{};
    if (!g_initialized) {
        return result;
    }

    padUpdate(&g_pad);
    result.buttons_down = padGetButtonsDown(&g_pad);
    result.buttons_held = padGetButtons(&g_pad);

    const HidAnalogStickState left = padGetStickPos(&g_pad, 0);
    const HidAnalogStickState right = padGetStickPos(&g_pad, 1);
    result.left_x = left.x;
    result.left_y = left.y;
    result.right_x = right.x;
    result.right_y = right.y;
    return result;
}

bool should_exit() {
    if (!appletMainLoop()) {
        return true;
    }
    const auto input = poll_input();
    return (input.buttons_down & HidNpadButton_Plus) != 0;
}

std::uint64_t monotonic_ticks() noexcept {
    return armGetSystemTick();
}

std::uint64_t tick_frequency() noexcept {
    return armGetSystemTickFreq();
}

void sleep_for_ns(std::int64_t nanoseconds) noexcept {
    if (nanoseconds > 0) {
        svcSleepThread(nanoseconds);
    }
}

bool submit_audio(const AudioBufferView&) noexcept {
    // Audren integration is intentionally a later slice. Returning false is a
    // clean capability failure and keeps SDL audio off the critical path.
    return false;
}

} // namespace mkw::horizon_runtime_services
