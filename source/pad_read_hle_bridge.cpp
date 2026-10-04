#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"
#include "hle/controller_status_contract.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <utility>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
using mkw::horizon_runtime_services::InputState;
namespace buttons = mkw::horizon_runtime_services::buttons;

std::int8_t Axis(std::int32_t value) {
    // libnx is positive-up. Scale like Aurora's signed 16-bit axes, bounding
    // both endpoints to the GC range before the caller's PADClampCircle2.
    return static_cast<std::int8_t>(std::clamp(value / 256, -127, 127));
}

PadStatusContract::Fields ConnectedStatus(const InputState& input) {
    PadStatusContract::Fields result{};
    constexpr std::array mappings{
        std::pair{buttons::A, 0x0100u},
        std::pair{buttons::B, 0x0200u},
        std::pair{buttons::X, 0x0400u},
        std::pair{buttons::Y, 0x0800u},
        std::pair{buttons::L, 0x0020u},
        std::pair{buttons::R, 0x0040u},
        std::pair{buttons::ZL | buttons::ZR, 0x0010u},
        std::pair{buttons::Plus, 0x1000u},
        std::pair{buttons::Left, 0x0001u},
        std::pair{buttons::Right, 0x0002u},
        std::pair{buttons::Down, 0x0004u},
        std::pair{buttons::Up, 0x0008u},
    };
    for (const auto& [host, guest] : mappings) {
        if (input.buttons_held & host) {
            result.buttons |= guest;
        }
    }
    result.stickX = Axis(input.left_x);
    result.stickY = Axis(input.left_y);
    result.substickX = Axis(input.right_x);
    result.substickY = Axis(input.right_y);
    result.triggerLeft = input.buttons_held & buttons::L ? 255u : 0u;
    result.triggerRight = input.buttons_held & buttons::R ? 255u : 0u;
    return result;
}

void Report(const char* status, std::uint32_t address, bool connected) {
    // This call occurs in the input loop. Write on transitions only, while
    // retaining the first successful buffer address for console diagnosis.
    static const char* previousStatus = nullptr;
    static std::uint32_t previousAddress = 0;
    static bool previousConnected = false;
    if (previousStatus == status && previousAddress == address && previousConnected == connected) {
        return;
    }
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-pad-read.txt", "w")) {
        std::fprintf(out, "status=%s\nbuffer=0x%08x\nbytes=48\nconnected_port0=%u\nrumble_mask=0\n",
                     status, address, connected ? 1u : 0u);
        std::fclose(out);
        previousStatus = status;
        previousAddress = address;
        previousConnected = connected;
    }
}
} // namespace

extern "C" void mkw_switch_hle_pad_read(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t address = cpu->gpr[3];
    mkw_switch_set_fast_track_stage("RMCP01_PAD_READ");
    cpu->gpr[3] = 0u; // No implemented rumble actuator capability.
    if (!address) {
        return; // Pinned null argument: no input polling or guest writes.
    }
    constexpr unsigned kChannels = 4;
    constexpr auto kBytes = kChannels * PadStatusContract::kGuestStatusSize;
    auto* destination = Memory::GetPointer(address, kBytes);
    if (!destination) {
        Report("invalid-range", address, false);
        return; // Validate all four slots before any poll or write.
    }
    const auto input = mkw::horizon_runtime_services::poll_input();
    for (unsigned port = 0; port < kChannels; ++port) {
        PadStatusContract::Fields fields{};
        if (port == 0 && input.connected) {
            fields = ConnectedStatus(input);
        } else {
            fields.error = -1; // PAD_ERR_NO_CONTROLLER, never a fabricated device.
        }
        const auto encoded = PadStatusContract::Encode(fields);
        std::memcpy(destination + port * encoded.size(), encoded.data(), encoded.size());
    }
    Report("read-pass", address, input.connected);
}

#endif
