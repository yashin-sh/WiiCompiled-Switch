#pragma once

#include "abi_bridge.h"
#include "memory.h"
#include "switch_dvd_hle_traits.hpp"

#include <cstdint>

// DVDLowClearCoverInterrupt (PAL 0x80166964). Pinned WiiCompiled does not
// emulate the Wii optical-drive cover interrupt on the host. The native
// override ignores the callback argument and reports the low-level request as
// accepted/ready immediately.
template <>
struct KnownNativeCpuCall<0x80166964u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

// DVDLowInquiry (PAL 0x80165A30). Pinned WiiCompiled acknowledges that the
// drive is present, marks the supplied DVD command block complete, completes
// the SDK cancel/reset bookkeeping, and returns 1 to indicate that the request
// was successfully issued. The callback argument is intentionally not invoked
// by this native override.
template <>
struct KnownNativeCpuCall<0x80165A30u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kCommandBlockStateOffset = 0x0Cu;
        const std::uint32_t commandBlock = cpu->gpr[3];
        if (commandBlock != 0u && commandBlock <= 0xFFFFFFF0u) {
            const std::uint32_t stateAddress = commandBlock + kCommandBlockStateOffset;
            if (Memory::Contains(stateAddress, 4u)) {
                Memory::Write32(stateAddress, 0u); // DVD_STATE_END
            }
        }

        mkw::dvd_hle_detail::InitializeCancelState();
        cpu->gpr[3] = 1u;
    }
};
