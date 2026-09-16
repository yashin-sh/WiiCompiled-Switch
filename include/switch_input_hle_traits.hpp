#pragma once

#include "abi_bridge.h"

#include <cstdint>

namespace mkw::switch_input_hle {

// Pinned WiiCompiled's WPAD HLE keeps only a tiny host-side library state at
// initialization time. Preserve that state now so later hardware-proven WPAD
// entry points can share it without constructing any Wii Bluetooth objects.
inline bool g_wpad_initialized = false;
inline std::uint8_t g_wpad_dpd_sensitivity = 3u;

} // namespace mkw::switch_input_hle

// WPADInit (PAL 0x801BF5C4). At the pinned WiiCompiled revision this only marks
// the WPAD contract initialized and returns WPAD_ERR_NONE / success (0). Do not
// pre-port probing, synchronization, callbacks, or physical controller state.
template <>
struct KnownNativeCpuCall<0x801BF5C4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        mkw::switch_input_hle::g_wpad_initialized = true;
        cpu->gpr[3] = 0u;
    }
};

// WPADGetDpdSensitivity (PAL 0x801C329C). Pinned WiiCompiled returns the shared
// WPAD stub state's DPD sensitivity, initialized to 3. No guest memory, device
// probing, or callback activity is involved at this boundary.
template <>
struct KnownNativeCpuCall<0x801C329Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        cpu->gpr[3] = static_cast<std::uint32_t>(mkw::switch_input_hle::g_wpad_dpd_sensitivity);
    }
};

// WPADGetStatus (PAL 0x801BF64C). Pinned WiiCompiled reports the shared WPAD
// contract state only: disabled (0) before initialization, ready (3) after
// WPADInit. The function takes no channel argument and does no device probing.
template <>
struct KnownNativeCpuCall<0x801BF64Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        cpu->gpr[3] = mkw::switch_input_hle::g_wpad_initialized ? 3u : 0u;
    }
};
