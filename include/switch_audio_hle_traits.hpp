#pragma once

#include "abi_bridge.h"

// Early Wii audio bootstrap is a native/HLE boundary in the pinned runtime.
// The desktop implementation starts the host audio backend and AX/DSP emulation,
// neither of which is required to prove boot-to-main on Horizon. Keep these
// entry points as deliberate guest-CPU no-ops for the fast-track; the real
// Switch audio backend will replace this boundary after first-frame bring-up.
//
// PAL addresses from pinned WiiCompiled a135beb...:
//   0x801A1138 __AIClockInit
//   0x801A1358 __OSInitAudioSystem
//   0x801A1520 __OSStopAudioSystem

template <>
struct KnownNativeCpuCall<0x801A1138u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};

template <>
struct KnownNativeCpuCall<0x801A1358u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};

template <>
struct KnownNativeCpuCall<0x801A1520u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};
