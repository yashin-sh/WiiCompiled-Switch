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

// Forecasted simple read-only NAND frontiers. These mirror pinned WiiCompiled
// wrappers over the already shared SD-backed synchronous runtime; no write,
// create, delete or safe-write commit path is enabled here.

template <>
struct KnownNativeCpuCall<0x8019BA04u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t fileInfoPtr = cpu->gpr[3];
        const std::int32_t offset =
            static_cast<std::int32_t>(cpu->gpr[4]);
        const std::int32_t whence =
            static_cast<std::int32_t>(cpu->gpr[5]);
        const std::uint32_t callbackPtr = cpu->gpr[6];
        const std::uint32_t commandBlockPtr = cpu->gpr[7];

        const std::int32_t result =
            mkw::switch_nand_runtime::SeekSync(
                fileInfoPtr, offset, whence);
        mkw::switch_nand_runtime::QueueCallback(
            callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(
            result < 0 ? result : 0);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x8019C048u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::int32_t result =
            mkw::switch_nand_runtime::GetLengthSync(
                cpu->gpr[3], cpu->gpr[4]);
        mkw::switch_nand_runtime::QueueCallback(
            cpu->gpr[5], result, cpu->gpr[6]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x8019D720u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::int32_t result =
            mkw::switch_nand_runtime::SafeCloseReadSync(cpu->gpr[3]);
        mkw::switch_nand_runtime::QueueCallback(
            cpu->gpr[4], result, cpu->gpr[5]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x8019E7B4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::int32_t result =
            mkw::switch_nand_runtime::GetTypeSync(
                cpu->gpr[3], cpu->gpr[4]);
        mkw::switch_nand_runtime::QueueCallback(
            cpu->gpr[5], result, cpu->gpr[6]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};
