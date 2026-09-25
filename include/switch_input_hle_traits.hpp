#pragma once

#include "abi_bridge.h"

#include <cstdint>

namespace mkw::switch_input_hle {

// Pinned WiiCompiled's WPAD HLE keeps only a tiny host-side library state at
// initialization time. Preserve that state now so later hardware-proven WPAD
// entry points can share it without constructing any Wii Bluetooth objects.
inline bool g_wpad_initialized = false;
inline std::uint8_t g_wpad_dpd_sensitivity = 3u;
inline std::uint32_t g_wpad_sync_device_callback = 0u;

// Pinned Aurora PADInit is idempotent host-side initialization. The desktop
// implementation also seeds SDL/keyboard mappings, which are not constructed
// on Horizon at this boundary. Preserve the initialization state so later
// hardware-proven PAD calls can share it without inventing controller devices.
inline bool g_pad_initialized = false;

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

// WPADSetSyncDeviceCallback (PAL 0x801BF640). Pinned WiiCompiled only swaps a
// host-side callback pointer: return the previous value in r3, then remember the
// new r3 argument. Do not invoke the callback or pre-port simple-sync behavior.
template <>
struct KnownNativeCpuCall<0x801BF640u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t callback = cpu->gpr[3];
        const std::uint32_t previous = mkw::switch_input_hle::g_wpad_sync_device_callback;
        mkw::switch_input_hle::g_wpad_sync_device_callback = callback;
        cpu->gpr[3] = previous;
    }
};

// WPADControlMotor (PAL 0x801C0EC4). At the pinned WiiCompiled revision this
// is intentionally a void no-op: channel and command are ignored, no WPAD state
// is mutated, and no return register is written. Preserve all guest GPRs.
template <>
struct KnownNativeCpuCall<0x801C0EC4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext*) noexcept {}
};

// PADInit (PAL 0x801AF2F0). Pinned WiiCompiled calls Aurora PADInit(), which is
// idempotent, marks its host PAD state initialized, seeds desktop keyboard
// bindings, and returns true. Horizon does not have those SDL keyboard objects
// at this boundary, so preserve only the proven initialization state and the
// guest-visible success value (1); do not pre-port PADRead or device mappings.
template <>
struct KnownNativeCpuCall<0x801AF2F0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        mkw::switch_input_hle::g_pad_initialized = true;
        cpu->gpr[3] = 1u;
    }
};
