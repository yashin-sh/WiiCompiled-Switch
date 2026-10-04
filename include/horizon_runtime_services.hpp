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

// Button masks retain libnx HID values. The Horizon implementation checks
// them against the SDK; consumers need no Switch or SDL headers.
namespace buttons {
inline constexpr std::uint64_t A = 1ull << 0;
inline constexpr std::uint64_t B = 1ull << 1;
inline constexpr std::uint64_t X = 1ull << 2;
inline constexpr std::uint64_t Y = 1ull << 3;
inline constexpr std::uint64_t L = 1ull << 6;
inline constexpr std::uint64_t R = 1ull << 7;
inline constexpr std::uint64_t ZL = 1ull << 8;
inline constexpr std::uint64_t ZR = 1ull << 9;
inline constexpr std::uint64_t Plus = 1ull << 10;
inline constexpr std::uint64_t Left = 1ull << 12;
inline constexpr std::uint64_t Up = 1ull << 13;
inline constexpr std::uint64_t Right = 1ull << 14;
inline constexpr std::uint64_t Down = 1ull << 15;
} // namespace buttons

struct InputState {
    std::uint64_t buttons_down = 0;
    std::uint64_t buttons_held = 0;
    std::int32_t left_x = 0;
    std::int32_t left_y = 0;
    std::int32_t right_x = 0;
    std::int32_t right_y = 0;
    bool connected = false;
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

// Runtime SD-card data is distinct from the build-time translated product.
// These roots contain only host/runtime state and must never be interpreted as
// a location from which translated game code is loaded.
std::filesystem::path application_root();
std::filesystem::path logs_root();
std::filesystem::path cache_root();
std::filesystem::path config_root();
std::filesystem::path nand_root();

bool submit_audio(const AudioBufferView& buffer) noexcept;

} // namespace mkw::horizon_runtime_services
