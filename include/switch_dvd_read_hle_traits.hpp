#pragma once

#include "abi_bridge.h"
#include "memory.h"
#include "switch_dvd_hle_traits.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

extern "C" void mkw_switch_gx_notify_guest_ram_dma_write(
    std::uint32_t address,
    std::uint32_t sizeBytes) noexcept;

namespace mkw::dvd_read_hle_detail {

constexpr std::uint32_t kDvdCbStateOffset = 0x0Cu;
constexpr std::uint32_t kDvdCbTransferredOffset = 0x20u;
constexpr std::uint32_t kDvdFileStartOffset = 0x30u;
constexpr std::uint32_t kDvdFileLengthOffset = 0x34u;
constexpr std::int32_t kDvdStateFatalError = -1;
constexpr std::int32_t kDvdStateEnd = 0;
constexpr std::int32_t kDvdFatalResult = -3;
constexpr std::uint32_t kMaxFstEntries = 0x00010000u;
constexpr std::size_t kMaxPathComponent = 255u;
constexpr const char* kDataFilesRoot =
    "sdmc:/switch/WiiCompiled-Switch/DATA/files/";
constexpr const char* kReadStatusPath =
    "sdmc:/switch/WiiCompiled-Switch/dvd-read-status.txt";

struct ResolvedFstFile {
    std::string relativePath;
    std::uint32_t fileStartWords = 0u;
    std::uint32_t fileSize = 0u;
    std::uint32_t extentBias = 0u;
    std::uint32_t entryIndex = 0u;
};

struct DirectoryFrame {
    std::uint32_t endIndex = 0u;
    std::string path;
};

inline std::uint32_t ReadFst32(std::uint32_t address) noexcept {
    try {
        return Memory::Read32(address);
    } catch (...) {
        return 0u;
    }
}

inline bool ReadFstName(
    std::uint32_t stringAddress,
    std::uint32_t fstEnd,
    std::string& out) noexcept {
    out.clear();
    try {
        for (std::size_t i = 0u; i < kMaxPathComponent; ++i) {
            const std::uint32_t address =
                stringAddress + static_cast<std::uint32_t>(i);
            if (address >= fstEnd || !Memory::Contains(address, 1u)) {
                return false;
            }
            const char ch = static_cast<char>(Memory::Read8(address));
            if (ch == '\0') {
                return true;
            }
            if (ch == '/' || ch == '\\') {
                return false;
            }
            out.push_back(ch);
        }
    } catch (...) {
        return false;
    }
    return false;
}

inline bool ResolvePublishedFile(
    std::uint32_t startWords,
    ResolvedFstFile& out) noexcept {
    if (!dvd_hle_detail::HasPublishedFst()) {
        return false;
    }

    constexpr std::uint32_t kFstAddressLowMem = 0x80000038u;
    constexpr std::uint32_t kFstSizeLowMem = 0x8000003Cu;

    try {
        const std::uint32_t fstAddress = Memory::Read32(kFstAddressLowMem);
        const std::uint32_t fstSize = Memory::Read32(kFstSizeLowMem);
        const std::uint64_t fstEnd64 =
            static_cast<std::uint64_t>(fstAddress) + fstSize;
        if (fstEnd64 > 0x1'0000'0000ull) {
            return false;
        }
        const std::uint32_t fstEnd = static_cast<std::uint32_t>(fstEnd64);

        const std::uint32_t entryCount = Memory::Read32(fstAddress + 8u);
        if (entryCount == 0u || entryCount > kMaxFstEntries) {
            return false;
        }

        const std::uint64_t stringBase64 =
            static_cast<std::uint64_t>(fstAddress) +
            static_cast<std::uint64_t>(entryCount) * 12u;
        if (stringBase64 >= fstEnd64) {
            return false;
        }
        const std::uint32_t stringBase =
            static_cast<std::uint32_t>(stringBase64);
        const std::uint64_t requestedStartBytes =
            static_cast<std::uint64_t>(startWords) * 4ull;

        std::vector<DirectoryFrame> directories;
        directories.reserve(16u);
        directories.push_back({entryCount, std::string()});

        for (std::uint32_t i = 1u; i < entryCount; ++i) {
            while (!directories.empty() &&
                   i >= directories.back().endIndex) {
                directories.pop_back();
            }
            if (directories.empty()) {
                return false;
            }

            const std::uint32_t entryAddress = fstAddress + i * 12u;
            const std::uint32_t nameWord = Memory::Read32(entryAddress);
            const std::uint8_t type =
                static_cast<std::uint8_t>(nameWord >> 24u);
            const std::uint32_t nameOffset = nameWord & 0x00FFFFFFu;

            const std::uint64_t nameAddress64 =
                static_cast<std::uint64_t>(stringBase) + nameOffset;
            if (nameAddress64 >= fstEnd64) {
                return false;
            }

            std::string name;
            if (!ReadFstName(
                    static_cast<std::uint32_t>(nameAddress64),
                    fstEnd,
                    name)) {
                return false;
            }

            if (type != 0u) {
                const std::uint32_t nextIndex =
                    Memory::Read32(entryAddress + 8u);
                if (nextIndex <= i || nextIndex > entryCount) {
                    return false;
                }

                std::string path = directories.back().path;
                if (!path.empty()) {
                    path.push_back('/');
                }
                path += name;
                directories.push_back({nextIndex, std::move(path)});
                continue;
            }

            const std::uint32_t fileStartWords =
                Memory::Read32(entryAddress + 4u);
            const std::uint32_t fileSize =
                Memory::Read32(entryAddress + 8u);
            const std::uint64_t fileStartBytes =
                static_cast<std::uint64_t>(fileStartWords) * 4ull;
            const std::uint64_t fileEndBytes = fileStartBytes + fileSize;

            const bool withinExtent =
                requestedStartBytes >= fileStartBytes &&
                (requestedStartBytes < fileEndBytes ||
                 (fileSize == 0u && requestedStartBytes == fileStartBytes));
            if (!withinExtent) {
                continue;
            }

            std::string path = directories.back().path;
            if (!path.empty()) {
                path.push_back('/');
            }
            path += name;

            out.relativePath = std::move(path);
            out.fileStartWords = fileStartWords;
            out.fileSize = fileSize;
            out.extentBias = static_cast<std::uint32_t>(
                requestedStartBytes - fileStartBytes);
            out.entryIndex = i;
            return true;
        }
    } catch (...) {
        return false;
    }
    return false;
}

inline void WriteReadStatus(
    const char* status,
    const ResolvedFstFile* file,
    std::uint32_t fileInfoPtr,
    std::uint32_t bufferPtr,
    std::int32_t length,
    std::int32_t offset,
    std::int32_t result) noexcept {
    static std::uint32_t eventCount = 0u;
    if (eventCount >= 32u) {
        return;
    }
    ++eventCount;

    FILE* out = std::fopen(
        kReadStatusPath,
        eventCount == 1u ? "w" : "a");
    if (!out) {
        return;
    }

    std::fprintf(
        out,
        "event=%u status=%s fileInfo=0x%08x buffer=0x%08x "
        "length=%d offset=%d result=%d",
        eventCount,
        status ? status : "<null>",
        fileInfoPtr,
        bufferPtr,
        length,
        offset,
        result);
    if (file) {
        std::fprintf(
            out,
            " entry=%u startWords=0x%08x bias=%u size=%u path=/%s",
            file->entryIndex,
            file->fileStartWords,
            file->extentBias,
            file->fileSize,
            file->relativePath.c_str());
    }
    std::fputc('\n', out);
    std::fclose(out);
}

inline std::int32_t FailRead(
    const char* status,
    const ResolvedFstFile* file,
    std::uint32_t fileInfoPtr,
    std::uint32_t bufferPtr,
    std::int32_t length,
    std::int32_t offset) noexcept {
    try {
        if (fileInfoPtr != 0u &&
            Memory::Contains(fileInfoPtr + kDvdCbTransferredOffset, 4u)) {
            Memory::Write32(
                fileInfoPtr + kDvdCbTransferredOffset,
                0u);
        }
        if (fileInfoPtr != 0u &&
            Memory::Contains(fileInfoPtr + kDvdCbStateOffset, 4u)) {
            Memory::Write32(
                fileInfoPtr + kDvdCbStateOffset,
                static_cast<std::uint32_t>(kDvdStateFatalError));
        }
    } catch (...) {
    }

    WriteReadStatus(
        status,
        file,
        fileInfoPtr,
        bufferPtr,
        length,
        offset,
        kDvdFatalResult);
    return kDvdFatalResult;
}

inline std::int32_t ReadPrio(
    std::uint32_t fileInfoPtr,
    std::uint32_t bufferPtr,
    std::int32_t length,
    std::int32_t offset) noexcept {
    if (fileInfoPtr == 0u ||
        !Memory::Contains(fileInfoPtr + kDvdFileLengthOffset, 4u)) {
        return FailRead(
            "invalid-file-info",
            nullptr,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    std::uint32_t startWords = 0u;
    std::uint32_t advertisedLength = 0u;
    try {
        startWords = Memory::Read32(fileInfoPtr + kDvdFileStartOffset);
        advertisedLength =
            Memory::Read32(fileInfoPtr + kDvdFileLengthOffset);
    } catch (...) {
        return FailRead(
            "file-info-read-failed",
            nullptr,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    ResolvedFstFile file{};
    if (!ResolvePublishedFile(startWords, file)) {
        return FailRead(
            "unresolved-fst-extent",
            nullptr,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    if (length < 0 || offset < 0) {
        return FailRead(
            "negative-range",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    const std::uint64_t requestedOffset64 =
        static_cast<std::uint64_t>(file.extentBias) +
        static_cast<std::uint32_t>(offset);
    if (requestedOffset64 > file.fileSize) {
        return FailRead(
            "offset-outside-file",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    const std::uint32_t requestedOffset =
        static_cast<std::uint32_t>(requestedOffset64);
    const std::uint32_t available = file.fileSize - requestedOffset;
    std::uint32_t readLength = static_cast<std::uint32_t>(length);
    if (readLength > available) {
        readLength = available;
    }
    if (advertisedLength != 0u &&
        static_cast<std::uint64_t>(offset) >= advertisedLength &&
        readLength != 0u) {
        return FailRead(
            "offset-outside-file-info",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }
    if (readLength != 0u &&
        !Memory::Contains(bufferPtr, readLength)) {
        return FailRead(
            "destination-unmapped",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    std::string hostPath(kDataFilesRoot);
    hostPath += file.relativePath;
    FILE* input = std::fopen(hostPath.c_str(), "rb");
    if (!input) {
        return FailRead(
            "host-file-open-failed",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    if (std::fseek(
            input,
            static_cast<long>(requestedOffset),
            SEEK_SET) != 0) {
        std::fclose(input);
        return FailRead(
            "host-file-seek-failed",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    std::array<std::uint8_t, 16u * 1024u> chunk{};
    std::uint32_t copied = 0u;
    while (copied < readLength) {
        const std::uint32_t remaining = readLength - copied;
        const std::size_t wanted =
            remaining < chunk.size() ? remaining : chunk.size();
        const std::size_t got =
            std::fread(chunk.data(), 1u, wanted, input);
        if (got != wanted) {
            std::fclose(input);
            return FailRead(
                "host-file-short-read",
                &file,
                fileInfoPtr,
                bufferPtr,
                length,
                offset);
        }

        try {
            for (std::size_t i = 0u; i < got; ++i) {
                Memory::Write8(
                    bufferPtr + copied +
                        static_cast<std::uint32_t>(i),
                    chunk[i]);
            }
        } catch (...) {
            std::fclose(input);
            return FailRead(
                "guest-copy-failed",
                &file,
                fileInfoPtr,
                bufferPtr,
                length,
                offset);
        }
        copied += static_cast<std::uint32_t>(got);
    }
    std::fclose(input);

    if (copied != 0u) {
        mkw_switch_gx_notify_guest_ram_dma_write(
            bufferPtr,
            copied);
    }

    try {
        Memory::Write32(
            fileInfoPtr + kDvdCbTransferredOffset,
            copied);
        Memory::Write32(
            fileInfoPtr + kDvdCbStateOffset,
            static_cast<std::uint32_t>(kDvdStateEnd));
        dvd_hle_detail::InitializeCancelState();
    } catch (...) {
        return FailRead(
            "completion-state-failed",
            &file,
            fileInfoPtr,
            bufferPtr,
            length,
            offset);
    }

    WriteReadStatus(
        "read-pass",
        &file,
        fileInfoPtr,
        bufferPtr,
        length,
        offset,
        static_cast<std::int32_t>(copied));
    return static_cast<std::int32_t>(copied);
}

inline void InvokeReadCallback(
    CpuContext* cpu,
    std::uint32_t callbackPtr,
    std::int32_t result,
    std::uint32_t fileInfoPtr) noexcept {
    if (!cpu || callbackPtr == 0u) {
        return;
    }

    try {
        CpuContext callbackCpu = *cpu;
        callbackCpu.gpr[3] = static_cast<std::uint32_t>(result);
        callbackCpu.gpr[4] = fileInfoPtr;
        CpuContextScope scope(&callbackCpu);
        InvokeIndirectCpu(callbackPtr, &callbackCpu);
    } catch (...) {
        WriteReadStatus(
            "callback-failed",
            nullptr,
            fileInfoPtr,
            0u,
            0,
            0,
            result);
    }
}

} // namespace mkw::dvd_read_hle_detail

// DVDReadPrio (PAL 0x8015E834). The pinned WiiCompiled native override maps
// DVDFileInfo::startAddr back to the published FST extent, reads the user's
// extracted DATA/files payload synchronously, copies it to guest RAM as a DMA
// write, and publishes transferredSize/state before returning the byte count.
template <>
struct KnownNativeCpuCall<0x8015E834u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t fileInfoPtr = cpu->gpr[3];
        const std::uint32_t bufferPtr = cpu->gpr[4];
        const std::int32_t length =
            static_cast<std::int32_t>(cpu->gpr[5]);
        const std::int32_t offset =
            static_cast<std::int32_t>(cpu->gpr[6]);

        cpu->gpr[3] = static_cast<std::uint32_t>(
            mkw::dvd_read_hle_detail::ReadPrio(
                fileInfoPtr,
                bufferPtr,
                length,
                offset));
    }
};

// Internal DVDReadAsyncPrio (PAL 0x8015E74C). Pinned WiiCompiled completes the
// local host read synchronously, invokes the guest callback immediately, then
// reports whether the request was accepted.
template <>
struct KnownNativeCpuCall<0x8015E74Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t fileInfoPtr = cpu->gpr[3];
        const std::uint32_t bufferPtr = cpu->gpr[4];
        const std::int32_t length =
            static_cast<std::int32_t>(cpu->gpr[5]);
        const std::int32_t offset =
            static_cast<std::int32_t>(cpu->gpr[6]);
        const std::uint32_t callbackPtr = cpu->gpr[7];

        const std::int32_t bytesRead =
            mkw::dvd_read_hle_detail::ReadPrio(
                fileInfoPtr,
                bufferPtr,
                length,
                offset);

        mkw::dvd_read_hle_detail::InvokeReadCallback(
            cpu,
            callbackPtr,
            bytesRead,
            fileInfoPtr);
        cpu->gpr[3] = bytesRead >= 0 ? 1u : 0u;
    }
};
