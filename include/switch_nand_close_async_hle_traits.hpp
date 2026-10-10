#pragma once

#include "abi_bridge.h"
#include "switch_nand_runtime.hpp"

#include <cstdint>

// NANDClose (PAL 0x8019CA80). Safe-open handles are deliberately rejected by
// the runtime and must travel through NANDSafeClose instead.
template <>
struct KnownNativeCpuCall<0x8019CA80u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_CLOSE");
        const std::int32_t result =
            mkw::switch_nand_runtime::CloseSync(cpu->gpr[3]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
    }
};

// NANDCloseAsync (PAL 0x8019CAEC). Pinned WiiCompiled forwards to synchronous
// NANDClose, queues the guest completion callback as (result, commandBlock),
// and returns that synchronous result verbatim.
template <>
struct KnownNativeCpuCall<0x8019CAECu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t fileInfoPtr = cpu->gpr[3];
        const std::uint32_t callbackPtr = cpu->gpr[4];
        const std::uint32_t commandBlockPtr = cpu->gpr[5];

        const std::int32_t result = mkw::switch_nand_runtime::CloseSync(fileInfoPtr);
        mkw::switch_nand_runtime::QueueCallback(callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};
