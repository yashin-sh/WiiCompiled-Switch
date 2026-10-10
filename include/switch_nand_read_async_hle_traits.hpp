#pragma once

#include "abi_bridge.h"
#include "switch_nand_runtime.hpp"

#include <cstdint>

// NANDRead (PAL 0x8019B7A4). The synchronous ABI returns the transfer count.
template <>
struct KnownNativeCpuCall<0x8019B7A4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_READ");
        const std::int32_t result = mkw::switch_nand_runtime::ReadSync(
            cpu->gpr[3], cpu->gpr[4], cpu->gpr[5]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
    }
};

// NANDReadAsync (PAL 0x8019B80C). Pinned WiiCompiled performs a synchronous
// NANDRead, queues the completion callback with the raw read result
// (bytesRead or a negative NAND error), then returns NAND_RESULT_OK when the
// synchronous read was non-negative. The transfer count therefore travels to
// the guest through the callback rather than r3.
template <>
struct KnownNativeCpuCall<0x8019B80Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t fileInfoPtr = cpu->gpr[3];
        const std::uint32_t bufferPtr = cpu->gpr[4];
        const std::uint32_t length = cpu->gpr[5];
        const std::uint32_t callbackPtr = cpu->gpr[6];
        const std::uint32_t commandBlockPtr = cpu->gpr[7];

        const std::int32_t result =
            mkw::switch_nand_runtime::ReadSync(fileInfoPtr, bufferPtr, length);
        mkw::switch_nand_runtime::QueueCallback(callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(result < 0 ? result : 0);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};
