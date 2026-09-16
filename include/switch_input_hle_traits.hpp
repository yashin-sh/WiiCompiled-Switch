#pragma once

#include "abi_bridge.h"

#include <cstdint>

namespace mkw::switch_input_hle {

// Pinned WiiCompiled's WPAD HLE keeps only a tiny host-side library state at
// initialization time. Preserve that state now so later hardware-proven WPAD
// entry points can share it without constructing any Wii Bluetooth objects.
inline bool g_wpad_initialized = false;

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
