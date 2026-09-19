#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace mkw::dvd_hle_detail {

inline bool IsGameCodeByte(std::uint8_t value) noexcept {
    return (value >= static_cast<std::uint8_t>('A') &&
            value <= static_cast<std::uint8_t>('Z')) ||
           (value >= static_cast<std::uint8_t>('0') &&
            value <= static_cast<std::uint8_t>('9'));
}

inline std::uint32_t CurrentDiscGameCode() noexcept {
    constexpr std::uint32_t kPalFallback = 0x524D4350u; // RMCP
    constexpr std::uint32_t kDiscHeader = 0x80000000u;

    if (!Memory::Contains(kDiscHeader, 4u)) {
        return kPalFallback;
    }

    const std::uint32_t value = Memory::Read32(kDiscHeader);
    for (int shift = 24; shift >= 0; shift -= 8) {
        const auto byte = static_cast<std::uint8_t>((value >> shift) & 0xFFu);
        if (!IsGameCodeByte(byte)) {
            return kPalFallback;
        }
    }
    return value;
}

inline bool HasPublishedFst() noexcept {
    constexpr std::uint32_t kFstAddressLowMem = 0x80000038u;
    constexpr std::uint32_t kFstSizeLowMem = 0x8000003Cu;
    constexpr std::uint32_t kMaxRuntimeFstSize = 0x00200000u;

    if (!Memory::Contains(kFstAddressLowMem, 8u)) {
        return false;
    }

    const std::uint32_t fstAddress = Memory::Read32(kFstAddressLowMem);
    const std::uint32_t fstSize = Memory::Read32(kFstSizeLowMem);
    if (fstAddress == 0u || fstSize < 12u || fstSize > kMaxRuntimeFstSize ||
        !Memory::Contains(fstAddress, fstSize)) {
        return false;
    }

    const std::uint32_t rootWord = Memory::Read32(fstAddress);
    const std::uint32_t entryCount = Memory::Read32(fstAddress + 8u);
    return (rootWord & 0xFF000000u) == 0x01000000u &&
           entryCount != 0u && entryCount <= 0x00010000u;
}

inline std::uint32_t ReadBigEndian32(const std::uint8_t* bytes) noexcept {
    return (static_cast<std::uint32_t>(bytes[0]) << 24u) |
           (static_cast<std::uint32_t>(bytes[1]) << 16u) |
           (static_cast<std::uint32_t>(bytes[2]) << 8u) |
           static_cast<std::uint32_t>(bytes[3]);
}

inline void WriteFstStatus(
    const char* status,
    std::uint32_t address = 0u,
    std::uint32_t size = 0u,
    std::uint32_t entries = 0u) noexcept {
    constexpr const char* kStatusPath =
        "sdmc:/switch/WiiCompiled-Switch/dvd-fst-status.txt";
    FILE* out = std::fopen(kStatusPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\naddress=0x%08x\nsize=%u\nentries=%u\n",
        status ? status : "<null>",
        address,
        size,
        entries);
    std::fclose(out);
}

inline bool HasLocalRmcp01Identity() noexcept {
    constexpr const char* kBootPath =
        "sdmc:/switch/WiiCompiled-Switch/DATA/sys/boot.bin";
    constexpr std::array<std::uint8_t, 6> kRmcp01 = {
        static_cast<std::uint8_t>('R'),
        static_cast<std::uint8_t>('M'),
        static_cast<std::uint8_t>('C'),
        static_cast<std::uint8_t>('P'),
        static_cast<std::uint8_t>('0'),
        static_cast<std::uint8_t>('1'),
    };

    FILE* boot = std::fopen(kBootPath, "rb");
    if (!boot) {
        WriteFstStatus("missing-boot-bin");
        return false;
    }

    std::array<std::uint8_t, kRmcp01.size()> identity{};
    const std::size_t read =
        std::fread(identity.data(), 1u, identity.size(), boot);
    std::fclose(boot);
    if (read != identity.size() ||
        std::memcmp(identity.data(), kRmcp01.data(), kRmcp01.size()) != 0) {
        WriteFstStatus("invalid-disc-identity");
        return false;
    }
    return true;
}

inline bool TryPublishLocalFst() noexcept {
    constexpr const char* kFstPath =
        "sdmc:/switch/WiiCompiled-Switch/DATA/sys/fst.bin";
    constexpr std::uint32_t kFstAddressLowMem = 0x80000038u;
    constexpr std::uint32_t kFstSizeLowMem = 0x8000003Cu;
    constexpr std::uint32_t kMem2ArenaHiLowMem = 0x80003128u;
    constexpr std::uint32_t kFstReservationSize = 0x00200000u;

    if (HasPublishedFst()) {
        return true;
    }
    if (!Memory::IsInitialized() ||
        !Memory::Contains(kFstAddressLowMem, 8u) ||
        !Memory::Contains(kMem2ArenaHiLowMem, 4u)) {
        WriteFstStatus("guest-memory-unavailable");
        return false;
    }
    if (!HasLocalRmcp01Identity()) {
        return false;
    }

    FILE* fst = std::fopen(kFstPath, "rb");
    if (!fst) {
        WriteFstStatus("missing-fst-bin");
        return false;
    }

    if (std::fseek(fst, 0, SEEK_END) != 0) {
        std::fclose(fst);
        WriteFstStatus("fst-seek-failed");
        return false;
    }
    const long fileSizeLong = std::ftell(fst);
    if (fileSizeLong < 12 ||
        static_cast<unsigned long>(fileSizeLong) > kFstReservationSize) {
        std::fclose(fst);
        WriteFstStatus("fst-size-invalid");
        return false;
    }
    const std::uint32_t fileSize =
        static_cast<std::uint32_t>(fileSizeLong);
    std::rewind(fst);

    std::array<std::uint8_t, 12> header{};
    if (std::fread(header.data(), 1u, header.size(), fst) != header.size()) {
        std::fclose(fst);
        WriteFstStatus("fst-header-read-failed");
        return false;
    }

    const std::uint32_t rootWord = ReadBigEndian32(header.data());
    const std::uint32_t entryCount = ReadBigEndian32(header.data() + 8u);
    const std::uint64_t entriesBytes =
        static_cast<std::uint64_t>(entryCount) * 12u;
    if ((rootWord & 0xFF000000u) != 0x01000000u ||
        entryCount == 0u || entryCount > 0x00010000u ||
        entriesBytes >= fileSize) {
        std::fclose(fst);
        WriteFstStatus("fst-structure-invalid", 0u, fileSize, entryCount);
        return false;
    }

    const std::uint32_t fstAddress = Memory::Read32(kMem2ArenaHiLowMem);
    if (fstAddress == 0u ||
        !Memory::Contains(fstAddress, kFstReservationSize)) {
        std::fclose(fst);
        WriteFstStatus(
            "fst-reservation-invalid",
            fstAddress,
            fileSize,
            entryCount);
        return false;
    }

    std::rewind(fst);
    std::array<std::uint8_t, 4096> chunk{};
    std::uint32_t copied = 0u;
    while (copied < fileSize) {
        const std::uint32_t remaining = fileSize - copied;
        const std::size_t wanted =
            remaining < chunk.size() ? remaining : chunk.size();
        if (std::fread(chunk.data(), 1u, wanted, fst) != wanted) {
            std::fclose(fst);
            WriteFstStatus(
                "fst-payload-read-failed",
                fstAddress,
                fileSize,
                entryCount);
            return false;
        }
        for (std::size_t i = 0u; i < wanted; ++i) {
            Memory::Write8(
                fstAddress + copied + static_cast<std::uint32_t>(i),
                chunk[i]);
        }
        copied += static_cast<std::uint32_t>(wanted);
    }
    std::fclose(fst);

    Memory::Write32(kFstAddressLowMem, fstAddress);
    Memory::Write32(kFstSizeLowMem, fileSize);
    if (!HasPublishedFst()) {
        Memory::Write32(kFstAddressLowMem, 0u);
        Memory::Write32(kFstSizeLowMem, 0u);
        WriteFstStatus(
            "fst-post-publish-validation-failed",
            fstAddress,
            fileSize,
            entryCount);
        return false;
    }

    WriteFstStatus("published", fstAddress, fileSize, entryCount);
    return true;
}

inline bool GuestBootstrapRangesAvailable() noexcept {
    return Memory::Contains(0x80000000u, 7u) &&
           Memory::Contains(0x80343230u, 0x20u) &&
           Memory::Contains(0x803434E0u, 0x80u) &&
           Memory::Contains(0x80386664u, 0xC2u);
}

inline void InitializeWaitingQueues() noexcept {
    constexpr std::uint32_t kQueueBase = 0x80343230u;
    for (std::uint32_t i = 0; i < 4u; ++i) {
        const std::uint32_t queue = kQueueBase + i * 8u;
        Memory::Write32(queue + 0u, queue);
        Memory::Write32(queue + 4u, queue);
    }
}

inline void InitializeCancelState() noexcept {
    InitializeWaitingQueues();

    Memory::Write32(0x80386664u, 0u); // Canceling
    Memory::Write32(0x80386668u, 0u); // ResumeFromHere
    Memory::Write32(0x80386670u, 0u); // PausingFlag
    Memory::Write32(0x8038667Cu, 1u); // CancelAllSync complete
    Memory::Write32(0x803866A8u, 1u); // PrepareReset complete
    Memory::Write32(0x803866F0u, 0u); // executing command block
}

inline void InitializeContexts() noexcept {
    constexpr std::uint32_t kContextBase = 0x803434E0u;
    constexpr std::uint32_t kContextStride = 0x20u;
    constexpr std::uint32_t kMagicValue = 0xFEEBDAEDu;

    for (std::uint32_t i = 0; i < 4u; ++i) {
        const std::uint32_t context = kContextBase + i * kContextStride;
        Memory::Write32(context + 0x0Cu, kMagicValue);
        Memory::Write32(context + 0x10u, i);
    }
}

inline void InitializeDiscHeader() noexcept {
    constexpr std::uint32_t kDiscHeader = 0x80000000u;
    Memory::Write32(kDiscHeader + 0x00u, CurrentDiscGameCode());
    Memory::Write16(kDiscHeader + 0x04u, 0x3031u); // maker code "01"
    Memory::Write8(kDiscHeader + 0x06u, 0x01u);    // disc number 1
}

} // namespace mkw::dvd_hle_detail

// DVDInit (PAL 0x8015EA1C). Pinned WiiCompiled replaces the Wii drive/IOS
// bootstrap with host-side DVD setup while still publishing substantial guest
// state: DVD flags, cancel/wait queues, context sentinels, the low-memory disc
// header, a runtime FST, then translated __DVDFSInit.
//
// Hardware evidence now shows the rendered RMCP01 path reaches main but not
// RKSystem::run/StaticR while the guest FST address and size remain zero. The
// dedicated resource gate is therefore active. Mirror the pinned publication
// contract from the user's own extracted RMCP01 DATA tree on SD: validate
// DATA/sys/boot.bin as RMCP01, copy DATA/sys/fst.bin into the 2 MiB MEM2 region
// already reserved below the IPC arena, publish low-memory address/size, then
// dispatch translated __DVDFSInit. No game bytes are embedded in this source or
// in public CI; absence/invalid local data leaves the previous safe path intact.
template <>
struct KnownNativeCpuCall<0x8015EA1Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu || !mkw::dvd_hle_detail::GuestBootstrapRangesAvailable()) {
            return;
        }

        constexpr std::uint32_t kDvdInitCalled = 0x803866A0u;
        if (Memory::Read8(kDvdInitCalled) != 0u) {
            return;
        }

        static bool initializing = false;
        if (initializing) {
            return;
        }
        initializing = true;

        Memory::Write8(0x80386724u, 1u);  // contexts initialized
        Memory::Write8(0x80386725u, 1u);  // low init called
        Memory::Write32(0x80386720u, 0u); // current context index

        mkw::dvd_hle_detail::InitializeCancelState();
        mkw::dvd_hle_detail::InitializeContexts();
        mkw::dvd_hle_detail::InitializeDiscHeader();

        if (!mkw::dvd_hle_detail::HasPublishedFst()) {
            mkw::dvd_hle_detail::TryPublishLocalFst();
        }
        if (mkw::dvd_hle_detail::HasPublishedFst()) {
            constexpr std::uint32_t kDvdFsInitAddress = 0x8015DF1Cu;
            InvokeIndirectCpu(kDvdFsInitAddress, cpu);
        }

        Memory::Write8(kDvdInitCalled, 1u);
        initializing = false;
    }
};
