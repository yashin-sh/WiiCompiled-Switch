#pragma once

#include "abi_bridge.h"
#include "memory.h"

// Early Wii audio bootstrap is a native/HLE boundary in the pinned runtime.
// The desktop implementation starts the host audio backend and AX/DSP emulation,
// neither of which is required to prove boot-to-main on Horizon. Keep these
// entry points as deliberate guest-CPU no-ops for the fast-track; the real
// Switch audio backend will replace this boundary after first-frame bring-up.
//
// PAL addresses from pinned WiiCompiled a135beb...:
//   0x801240B0 AIInit
//   0x801A1138 __AIClockInit
//   0x801A1358 __OSInitAudioSystem
//   0x801A1520 __OSStopAudioSystem

// Real-Switch hardware reached PAL AIInit (0x801240B0) with r3=0 after the
// fourth KD request close was crossed. Pinned WiiCompiled initializes the
// guest-visible AI globals on the first call and separately starts a desktop
// host audio backend. Mirror only the guest state here: Horizon audio remains
// outside this exact blocker, and adjacent AI entry points stay unsupported
// until hardware reaches them.
template <>
struct KnownNativeCpuCall<0x801240B0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kAIInitializedAddr = 0x80386448u;
        constexpr std::uint32_t kAICallbackBusyAddr = 0x8038644Cu;
        constexpr std::uint32_t kAICallbackStackSwitchAddr = 0x8038647Cu;
        constexpr std::uint32_t kAIDmaCallbackAddr = 0x80386480u;

        try {
            if (!Memory::Contains(kAIInitializedAddr, 4u) ||
                !Memory::Contains(kAICallbackBusyAddr, 4u) ||
                !Memory::Contains(kAICallbackStackSwitchAddr, 4u) ||
                !Memory::Contains(kAIDmaCallbackAddr, 4u)) {
                return;
            }

            if (Memory::Read32(kAIInitializedAddr) == 1u) {
                return;
            }

            Memory::Write32(kAIDmaCallbackAddr, 0u);
            Memory::Write32(kAICallbackBusyAddr, 0u);
            Memory::Write32(kAICallbackStackSwitchAddr, cpu->gpr[3]);
            Memory::Write32(kAIInitializedAddr, 1u);
        } catch (...) {
            // Pinned WiiCompiled uses TryRead/TryWrite for this bookkeeping;
            // an unavailable guest range therefore leaves AIInit best-effort.
        }
    }
};

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
