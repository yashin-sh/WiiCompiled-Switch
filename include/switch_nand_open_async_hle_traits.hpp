#pragma once

#include "abi_bridge.h"
#include "switch_nand_runtime.hpp"

#include <cstdint>
#include <cstdlib>

// NANDOpenAsync (PAL 0x8019C918). Pinned WiiCompiled forwards the request to
// synchronous NANDOpen, queues the guest completion callback as
// (result, commandBlock), and returns the same result. Reuse the same SD-backed
// open/callback bridge as NANDPrivateOpenAsync so both public/private entry
// points observe identical guest-visible open semantics.
template <>
struct KnownNativeCpuCall<0x8019C918u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t pathPtr = cpu->gpr[3];
        const std::uint32_t fileInfoPtr = cpu->gpr[4];
        const std::uint32_t mode = cpu->gpr[5];
        const std::uint32_t callbackPtr = cpu->gpr[6];
        const std::uint32_t commandBlockPtr = cpu->gpr[7];

        const std::int32_t result =
            mkw::switch_nand_runtime::OpenSync(pathPtr, fileInfoPtr, mode);
        mkw::switch_nand_runtime::QueueCallback(callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

// NANDPrivateSafeOpenAsync (PAL 0x8019D104). The latest real-Switch RFL path
// reaches the read-only safe-open variant (mode=1). Pinned WiiCompiled ignores
// the temporary buffer for this mode, opens the original file in place, marks
// NANDFileInfo as a safe-open handle, queues (result, commandBlock), and returns
// the same result. Keep write modes explicit hardware frontiers instead of
// pre-porting shadow-copy/commit behavior that this run has not exercised.
template <>
struct KnownNativeCpuCall<0x8019D104u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        constexpr std::uint32_t kAddress = 0x8019D104u;
        constexpr std::uint32_t kObservedMode = 1u;

        const std::uint32_t pathPtr = cpu->gpr[3];
        const std::uint32_t fileInfoPtr = cpu->gpr[4];
        const std::uint32_t mode = cpu->gpr[5];
        const std::uint32_t callbackPtr = cpu->gpr[8];
        const std::uint32_t commandBlockPtr = cpu->gpr[9];

        if (mode != kObservedMode) {
            mkw_switch_report_unsupported_translated_dispatch(
                "NAND_PRIVATE_SAFE_OPEN_ASYNC_UNPROVEN_MODE",
                kAddress,
                cpu);
            std::abort();
        }

        const std::int32_t result =
            mkw::switch_nand_runtime::SafeOpenReadSync(pathPtr, fileInfoPtr);
        mkw::switch_nand_runtime::QueueCallback(
            callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

