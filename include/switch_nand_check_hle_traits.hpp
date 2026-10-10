#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

// NANDCheck (PAL 0x8019EAD0). The pinned SD-backed NAND implementation
// ignores blockSize/blockCount and writes one healthy-result word. Preserve
// its four-byte range check and -8 result on an invalid/unwritable output.
// This is the virtual NAND contract, not a physical free-space measurement.
template <>
struct KnownNativeCpuCall<0x8019EAD0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        mkw_switch_set_fast_track_stage("RMCP01_NAND_CHECK");
        const std::uint32_t output = cpu->gpr[5];
        std::uint32_t result = static_cast<std::uint32_t>(-8);
        if (output != 0 && Memory::Contains(output, sizeof(std::uint32_t))) {
            try {
                Memory::Write32(output, 0u);
                result = 0u;
            } catch (const Memory::AccessViolation&) {
                // Match NANDCheck_HLE's guest-visible write failure.
            }
        }
        cpu->gpr[3] = result;
    }
};
