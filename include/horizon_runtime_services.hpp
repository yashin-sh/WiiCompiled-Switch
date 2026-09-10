#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace mkw::horizon_runtime_services {

enum class BackendState : std::uint8_t {
    Ready,
    Stubbed,
};

struct Status {
    bool lifecycle_ready = false;
    bool filesystem_ready = false;
    bool timing_ready = false;
    bool input_ready = false;
    BackendState audio = BackendState::Stubbed;
    BackendState graphics = BackendState::Stubbed;
};

struct InputState {
    std::uint64_t buttons_down = 0;
    std::uint64_t buttons_held = 0;
    std::int32_t left_x = 0;
    std::int32_t left_y = 0;
    std::int32_t right_x = 0;
    std::int32_t right_y = 0;
};

struct AudioBufferView {
    const std::int16_t* interleaved_samples = nullptr;
    std::size_t frames = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t channels = 0;
};

// SDL-free Horizon host-services boundary. The first runtime bring-up only
// requires lifecycle, filesystem, timing and input. Audio and graphics expose
// explicit stubs so callers fail deliberately instead of falling through to
// desktop SDL/Aurora assumptions.
Status initialize();
Status status() noexcept;
void shutdown() noexcept;

bool should_exit();
InputState poll_input();

std::uint64_t monotonic_ticks() noexcept;
std::uint64_t tick_frequency() noexcept;
void sleep_for_ns(std::int64_t nanoseconds) noexcept;

std::filesystem::path application_root();
std::filesystem::path user_game_data_root();

bool submit_audio(const AudioBufferView& buffer) noexcept;

} // namespace mkw::horizon_runtime_services
