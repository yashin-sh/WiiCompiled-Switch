#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>

extern "C" void mkw_switch_hle_os_create_thread(CpuContext* cpu) noexcept;

// OSGetCurrentThread (PAL 0x801A98B0). Pinned WiiCompiled registers this as a
// native function. Its complete guest-visible behavior is to return the running
// guest thread/context pointer from low memory at 0x800000E4, or null if the
// guest word cannot be read.
template <>
struct KnownNativeCpuCall<0x801A98B0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;
        try {
            cpu->gpr[3] = Memory::Read32(kOSRunningContextAddr);
        } catch (...) {
            cpu->gpr[3] = 0u;
        }
    }
};

// OSCreateThread (PAL 0x801A9E84). Pinned WiiCompiled creates a host fiber only
// when its desktop GuestFiberManager is present, but the guest OSThread/context,
// stack markers, priorities, scheduler slow-path state and global thread-list
// linkage are independent guest-visible semantics. The Switch bridge mirrors
// those semantics without fabricating a desktop host fiber.
template <>
struct KnownNativeCpuCall<0x801A9E84u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_create_thread(cpu);
    }
};
