#pragma once

#include "abi_bridge.h"
#include "switch_nand_runtime.hpp"

#include <cstdint>

// Synchronous NANDCreate/NANDWrite stay on the observed, bounded registry
// bridges. A broad compile-time trait would silently supersede their guards.

// NANDPrivateCreateAsync (PAL 0x8019B524).
template <>
struct KnownNativeCpuCall<0x8019B524u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_CREATE_ASYNC");
        const std::int32_t result = mkw::switch_nand_runtime::CreateSync(
            cpu->gpr[3], cpu->gpr[4], cpu->gpr[5]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[6], result, cpu->gpr[7]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

// NANDDelete (PAL 0x8019B59C).
template <>
struct KnownNativeCpuCall<0x8019B59Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_DELETE");
        const std::int32_t result =
            mkw::switch_nand_runtime::DeleteSync(cpu->gpr[3]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
    }
};

// NANDPrivateDeleteAsync (PAL 0x8019B6E4).
template <>
struct KnownNativeCpuCall<0x8019B6E4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_DELETE_ASYNC");
        const std::int32_t result =
            mkw::switch_nand_runtime::DeleteSync(cpu->gpr[3]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[4], result, cpu->gpr[5]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

// NANDWriteAsync (PAL 0x8019B8EC). As on pinned WiiCompiled, the byte count is
// delivered to the callback; a non-negative synchronous result collapses to OK
// in r3.
template <>
struct KnownNativeCpuCall<0x8019B8ECu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_WRITE_ASYNC");
        const std::int32_t result = mkw::switch_nand_runtime::WriteSync(
            cpu->gpr[3], cpu->gpr[4], cpu->gpr[5]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[6], result, cpu->gpr[7]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result < 0 ? result : 0);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

// NANDCreateDir (PAL 0x8019BBE0).
template <>
struct KnownNativeCpuCall<0x8019BBE0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_CREATE_DIR");
        const std::int32_t result = mkw::switch_nand_runtime::CreateDirSync(
            cpu->gpr[3], cpu->gpr[4], cpu->gpr[5]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
    }
};

// NANDPrivateCreateDirAsync (PAL 0x8019BCC8).
template <>
struct KnownNativeCpuCall<0x8019BCC8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_CREATE_DIR_ASYNC");
        const std::int32_t result = mkw::switch_nand_runtime::CreateDirSync(
            cpu->gpr[3], cpu->gpr[4], cpu->gpr[5]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[6], result, cpu->gpr[7]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

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

// NANDSafeOpen (PAL 0x8019CB74). The temporary guest buffer is an SDK detail;
// the Horizon backend uses a same-directory host shadow file instead.
template <>
struct KnownNativeCpuCall<0x8019CB74u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_SAFE_OPEN");
        const std::int32_t result = mkw::switch_nand_runtime::SafeOpenSync(
            cpu->gpr[3], cpu->gpr[4], cpu->gpr[5]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
    }
};

// NANDPrivateSafeOpenAsync (PAL 0x8019D104). Pinned WiiCompiled forwards to
// the same synchronous NANDSafeOpen implementation before queueing the guest
// callback, so the file-info safe-open flag is intentionally left untouched.
// Write/read-write modes use the same sibling shadow/commit path as sync open.
template <>
struct KnownNativeCpuCall<0x8019D104u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_SAFE_OPEN_ASYNC");

        const std::uint32_t pathPtr = cpu->gpr[3];
        const std::uint32_t fileInfoPtr = cpu->gpr[4];
        const std::uint32_t mode = cpu->gpr[5];
        const std::uint32_t callbackPtr = cpu->gpr[8];
        const std::uint32_t commandBlockPtr = cpu->gpr[9];

        const std::int32_t result =
            mkw::switch_nand_runtime::SafeOpenSync(pathPtr, fileInfoPtr, mode);
        mkw::switch_nand_runtime::QueueCallback(callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

// NANDSafeClose (PAL 0x8019CF28).
template <>
struct KnownNativeCpuCall<0x8019CF28u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_SAFE_CLOSE");
        const std::int32_t result =
            mkw::switch_nand_runtime::SafeCloseSync(cpu->gpr[3]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
    }
};

// NANDSafeCloseAsync (PAL 0x8019D720).
template <>
struct KnownNativeCpuCall<0x8019D720u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_NAND_SAFE_CLOSE_ASYNC");
        const std::int32_t result =
            mkw::switch_nand_runtime::SafeCloseSync(cpu->gpr[3]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[4], result, cpu->gpr[5]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};

// Forecasted simple read-only NAND frontiers. These mirror pinned WiiCompiled
// wrappers over the already shared SD-backed synchronous runtime.
template <>
struct KnownNativeCpuCall<0x8019BA04u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t fileInfoPtr = cpu->gpr[3];
        const std::int32_t offset = static_cast<std::int32_t>(cpu->gpr[4]);
        const std::int32_t whence = static_cast<std::int32_t>(cpu->gpr[5]);
        const std::uint32_t callbackPtr = cpu->gpr[6];
        const std::uint32_t commandBlockPtr = cpu->gpr[7];

        const std::int32_t result =
            mkw::switch_nand_runtime::SeekSync(fileInfoPtr, offset, whence);
        mkw::switch_nand_runtime::QueueCallback(callbackPtr, result, commandBlockPtr);
        cpu->gpr[3] = static_cast<std::uint32_t>(result < 0 ? result : 0);
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
            mkw::switch_nand_runtime::GetLengthSync(cpu->gpr[3], cpu->gpr[4]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[5], result, cpu->gpr[6]);
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
            mkw::switch_nand_runtime::GetTypeSync(cpu->gpr[3], cpu->gpr[4]);
        mkw::switch_nand_runtime::QueueCallback(cpu->gpr[5], result, cpu->gpr[6]);
        cpu->gpr[3] = static_cast<std::uint32_t>(result);
        mkw::switch_nand_runtime::PumpCallbacks(cpu);
    }
};
