#include "switch_nand_runtime.hpp"
#include "switch_nand_write_runtime.hpp"

#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace mkw::switch_nand_runtime {
namespace {

constexpr std::int32_t kResultOk = 0;
constexpr std::int32_t kResultExists = -6;
constexpr std::int32_t kResultInvalid = -8;
constexpr std::int32_t kResultNoExists = -12;
constexpr std::int32_t kResultUnknown = -64;
constexpr std::size_t kAccessTypeOffset = 0x88u;
constexpr std::size_t kOpenFlagOffset = 0x8au;
constexpr std::uint8_t kOpenFlag = 1u;
constexpr std::uint8_t kClosedFlag = 2u;
constexpr std::uint8_t kSafeOpenFlag = 3u;
constexpr std::uint8_t kSafeClosedFlag = 4u;
constexpr std::uint8_t kSafeOpenAsyncFlag = 5u;
constexpr std::uint8_t kSafeClosedAsyncFlag = 6u;

struct FileHandle {
    FILE* file = nullptr;
    std::filesystem::path path;
    std::int32_t mode = 0;
    // Non-empty for a crash-safe shadow handle. The guest writes to `path` and
    // SafeCloseSync atomically publishes that sibling file over this target.
    std::filesystem::path safeCommitPath;
};

std::mutex g_fileMutex;
std::map<std::int32_t, FileHandle> g_fileHandles;
std::int32_t g_nextFd = 100;

std::uint32_t CurrentGameCode() noexcept {
    constexpr std::uint32_t kFallback = 0x524D4350u;
    if (!Memory::Contains(0x80000000u, 4u)) {
        return kFallback;
    }
    const std::uint32_t code = Memory::Read32(0x80000000u);
    for (int shift = 24; shift >= 0; shift -= 8) {
        const unsigned char ch = static_cast<unsigned char>((code >> shift) & 0xffu);
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z'))) {
            return kFallback;
        }
    }
    return code;
}

std::string CurrentDataDir() {
    char path[64] = {};
    std::snprintf(path, sizeof(path), "/title/00010004/%08x/data", CurrentGameCode());
    return path;
}

bool ReadGuestCString(std::uint32_t address, std::string& out) noexcept {
    out.clear();
    if (address == 0u) {
        return false;
    }
    for (std::size_t i = 0; i < 1024u; ++i) {
        const std::uint32_t current = address + static_cast<std::uint32_t>(i);
        if (!Memory::Contains(current, 1u)) {
            return false;
        }
        const char ch = static_cast<char>(Memory::Read8(current));
        if (ch == '\0') {
            return !out.empty();
        }
        out.push_back(ch);
    }
    return false;
}

std::string Normalize(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.empty()) {
        return {};
    }
    if (path.front() != '/') {
        path = CurrentDataDir() + "/" + path;
    }

    std::vector<std::string> parts;
    std::string part;
    for (std::size_t i = 1u; i <= path.size(); ++i) {
        if (i < path.size() && path[i] != '/') {
            part.push_back(path[i]);
            continue;
        }
        if (part == "..") {
            if (!parts.empty()) {
                parts.pop_back();
            }
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        part.clear();
    }

    std::string result;
    for (const auto& item : parts) {
        result += "/" + item;
    }
    return result.empty() ? "/" : result;
}

std::filesystem::path HostPath(const std::string& guestPath) {
    std::filesystem::path host = mkw::horizon_runtime_services::nand_root();
    std::size_t cursor = 0u;
    while (cursor < guestPath.size()) {
        const std::size_t slash = guestPath.find('/', cursor);
        const std::size_t end = slash == std::string::npos ? guestPath.size() : slash;
        if (end != cursor) {
            host /= guestPath.substr(cursor, end - cursor);
        }
        if (slash == std::string::npos) {
            break;
        }
        cursor = slash + 1u;
    }
    return host;
}

bool ResolveHostPath(std::uint32_t pathPtr, std::filesystem::path& out) noexcept {
    try {
        std::string guestPath;
        if (!ReadGuestCString(pathPtr, guestPath)) {
            return false;
        }
        const std::string normalized = Normalize(guestPath);
        if (normalized.empty()) {
            return false;
        }
        out = HostPath(normalized);
        return true;
    } catch (...) {
        return false;
    }
}

std::filesystem::path SafeTempPathFor(const std::filesystem::path& path) {
    std::filesystem::path temp = path;
    temp += ".nandsafe.tmp";
    return temp;
}

bool FlushToDisk(FILE* file) noexcept {
    if (!file || std::fflush(file) != 0) {
        return false;
    }
#if !defined(_WIN32)
    const int fd = fileno(file);
    if (fd < 0 || fsync(fd) != 0) {
        return false;
    }
#endif
    return true;
}

std::int32_t AllocateHandle(FILE* file,
                            const std::filesystem::path& path,
                            std::uint32_t mode,
                            const std::filesystem::path& commitPath = {}) {
    std::lock_guard<std::mutex> lock(g_fileMutex);
    const std::int32_t fd = g_nextFd++;
    try {
        g_fileHandles.emplace(
            fd, FileHandle{file, path, static_cast<std::int32_t>(mode), commitPath});
    } catch (...) {
        std::fclose(file);
        throw;
    }
    return fd;
}

} // namespace

std::int32_t OpenSync(std::uint32_t pathPtr,
                      std::uint32_t fileInfoPtr,
                      std::uint32_t mode) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u) ||
        (mode != 1u && mode != 2u && mode != 3u)) {
        return kResultInvalid;
    }

    try {
        std::filesystem::path hostPath;
        if (!ResolveHostPath(pathPtr, hostPath)) {
            return kResultInvalid;
        }

        FILE* file = nullptr;
        if (mode == 1u) {
            file = std::fopen(hostPath.c_str(), "rb");
        } else {
            file = std::fopen(hostPath.c_str(), "r+b");
            if (!file) {
                std::error_code ec;
                std::filesystem::create_directories(hostPath.parent_path(), ec);
                if (ec) {
                    return kResultUnknown;
                }
                file = std::fopen(hostPath.c_str(), "w+b");
            }
        }
        if (!file) {
            return mode == 1u ? kResultNoExists : kResultUnknown;
        }

        const std::int32_t fd = AllocateHandle(file, hostPath, mode);
        Memory::Write32(fileInfoPtr, static_cast<std::uint32_t>(fd));
        Memory::Write8(
            fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset), kOpenFlag);
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t SafeOpenReadSync(std::uint32_t pathPtr,
                              std::uint32_t fileInfoPtr) noexcept {
    if (fileInfoPtr == 0u ||
        !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u)) {
        return kResultInvalid;
    }

    try {
        Memory::Write8(
            fileInfoPtr + static_cast<std::uint32_t>(kAccessTypeOffset), 1u);

        const std::int32_t result = OpenSync(pathPtr, fileInfoPtr, 1u);
        if (result == kResultOk) {
            Memory::Write8(
                fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset),
                kSafeOpenFlag);
        }
        return result;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t SafeOpenSync(std::uint32_t pathPtr,
                          std::uint32_t fileInfoPtr,
                          std::uint32_t mode) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u) ||
        (mode != 1u && mode != 2u && mode != 3u)) {
        return kResultInvalid;
    }

    try {
        Memory::Write8(
            fileInfoPtr + static_cast<std::uint32_t>(kAccessTypeOffset),
            static_cast<std::uint8_t>(mode));

        if (mode == 1u) {
            return SafeOpenReadSync(pathPtr, fileInfoPtr);
        }

        std::filesystem::path hostPath;
        if (!ResolveHostPath(pathPtr, hostPath)) {
            return kResultInvalid;
        }

        std::error_code ec;
        if (!std::filesystem::exists(hostPath, ec)) {
            return ec ? kResultUnknown : kResultNoExists;
        }
        if (std::filesystem::is_directory(hostPath, ec)) {
            return kResultInvalid;
        }
        if (ec) {
            return kResultUnknown;
        }

        const std::filesystem::path tempPath = SafeTempPathFor(hostPath);
        // A scratch file owned by this process is not a stale file from a crash.
        // Keep this lock through publication of the handle to exclude a second opener.
        std::lock_guard<std::mutex> lock(g_fileMutex);
        for (const auto& [fd, handle] : g_fileHandles) {
            (void)fd;
            if (handle.file && handle.path == tempPath) {
                return kResultInvalid;
            }
        }
        ec.clear();
        if (std::filesystem::exists(tempPath, ec)) {
            std::filesystem::remove(tempPath, ec);
            if (ec) {
                return kResultUnknown;
            }
        } else if (ec) {
            return kResultUnknown;
        }

        ec.clear();
        std::filesystem::copy_file(
            hostPath,
            tempPath,
            std::filesystem::copy_options::overwrite_existing,
            ec);
        if (ec) {
            return kResultUnknown;
        }

        FILE* file = std::fopen(tempPath.c_str(), "r+b");
        if (!file) {
            std::filesystem::remove(tempPath, ec);
            return kResultUnknown;
        }

        const std::int32_t fd = g_nextFd++;
        try {
            g_fileHandles.emplace(fd, FileHandle{file, tempPath, static_cast<std::int32_t>(mode), hostPath});
        } catch (...) {
            std::fclose(file);
            std::filesystem::remove(tempPath, ec);
            throw;
        }
        Memory::Write32(fileInfoPtr, static_cast<std::uint32_t>(fd));
        Memory::Write8(
            fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset),
            kSafeOpenFlag);
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t ReadSync(std::uint32_t fileInfoPtr,
                      std::uint32_t bufferPtr,
                      std::uint32_t length) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, 4u) ||
        bufferPtr == 0u || !Memory::Contains(bufferPtr, length == 0u ? 1u : length)) {
        return kResultInvalid;
    }

    try {
        auto* buffer = static_cast<std::uint8_t*>(Memory::GetPointer(bufferPtr));
        if (!buffer) {
            return kResultInvalid;
        }

        const std::int32_t fd = static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        std::lock_guard<std::mutex> lock(g_fileMutex);
        const auto it = g_fileHandles.find(fd);
        if (it == g_fileHandles.end() || !it->second.file) {
            return kResultInvalid;
        }

        const std::size_t bytesRead = std::fread(buffer, 1u, length, it->second.file);
        return static_cast<std::int32_t>(bytesRead);
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t WriteSync(std::uint32_t fileInfoPtr,
                       std::uint32_t bufferPtr,
                       std::uint32_t length) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, 4u) ||
        bufferPtr == 0u || !Memory::Contains(bufferPtr, length == 0u ? 1u : length)) {
        return kResultInvalid;
    }

    try {
        const auto* buffer = static_cast<const std::uint8_t*>(Memory::GetPointer(bufferPtr));
        if (!buffer) {
            return kResultInvalid;
        }

        const std::int32_t fd = static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        std::lock_guard<std::mutex> lock(g_fileMutex);
        const auto it = g_fileHandles.find(fd);
        if (it == g_fileHandles.end() || !it->second.file) {
            return kResultInvalid;
        }

        const std::size_t bytesWritten =
            std::fwrite(buffer, 1u, length, it->second.file);
        // Pinned NANDWrite returns fwrite's count even when the later flush fails.
        std::fflush(it->second.file);
        return static_cast<std::int32_t>(bytesWritten);
    } catch (...) {
        return kResultInvalid;
    }
}

BannerWriteResult WriteBannerSync(std::uint32_t fileInfoPtr,
                                  std::uint32_t bufferPtr,
                                  std::uint32_t length) noexcept {
    BannerWriteResult out;
    if (length != 0x72a0u || fileInfoPtr == 0u ||
        !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u) ||
        bufferPtr == 0u || !Memory::Contains(bufferPtr, length)) {
        return out;
    }
    try {
        out.fd = static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        out.openFlag = Memory::Read8(fileInfoPtr + kOpenFlagOffset);
        const auto expected = mkw::horizon_runtime_services::nand_root() / "tmp" / "banner.bin";
        std::lock_guard<std::mutex> lock(g_fileMutex);
        const auto it = g_fileHandles.find(out.fd);
        if (it == g_fileHandles.end() || !it->second.file) {
            return out;
        }
        out.mode = it->second.mode;
        if (out.openFlag != 1u || out.mode != 2 || it->second.path != expected ||
            std::ftell(it->second.file) != 0) {
            return out;
        }
        const auto* buffer = static_cast<const std::uint8_t*>(Memory::GetPointer(bufferPtr));
        if (!buffer) {
            return out;
        }
        out.admitted = true;
        out.result = static_cast<std::int32_t>(std::fwrite(buffer, 1u, length, it->second.file));
        // Pinned NANDWrite returns fwrite's count and ignores fflush's result.
        std::fflush(it->second.file);
        return out;
    } catch (...) {
        return out;
    }
}

std::int32_t SeekSync(std::uint32_t fileInfoPtr,
                      std::int32_t offset,
                      std::int32_t whence) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, 4u)) {
        return kResultInvalid;
    }

    try {
        const std::int32_t fd =
            static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        std::lock_guard<std::mutex> lock(g_fileMutex);
        const auto it = g_fileHandles.find(fd);
        if (it == g_fileHandles.end() || !it->second.file) {
            return kResultInvalid;
        }

        int origin = SEEK_SET;
        if (whence == 1) {
            origin = SEEK_CUR;
        } else if (whence == 2) {
            origin = SEEK_END;
        }

        if (std::fseek(it->second.file, offset, origin) != 0) {
            return kResultUnknown;
        }
        const long position = std::ftell(it->second.file);
        return position < 0 ? kResultUnknown : static_cast<std::int32_t>(position);
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t GetLengthSync(std::uint32_t fileInfoPtr,
                           std::uint32_t outLengthPtr) noexcept {
    if (fileInfoPtr == 0u || outLengthPtr == 0u ||
        !Memory::Contains(fileInfoPtr, 4u) ||
        !Memory::Contains(outLengthPtr, 4u)) {
        return kResultInvalid;
    }

    try {
        const std::int32_t fd =
            static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        std::lock_guard<std::mutex> lock(g_fileMutex);
        const auto it = g_fileHandles.find(fd);
        if (it == g_fileHandles.end() || !it->second.file) {
            return kResultInvalid;
        }

        const long original = std::ftell(it->second.file);
        if (original < 0 || std::fseek(it->second.file, 0, SEEK_END) != 0) {
            return kResultUnknown;
        }

        const long end = std::ftell(it->second.file);
        const bool restored = std::fseek(it->second.file, original, SEEK_SET) == 0;
        if (end < 0 || !restored) {
            return kResultUnknown;
        }

        Memory::Write32(outLengthPtr, static_cast<std::uint32_t>(end));
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t CloseSync(std::uint32_t fileInfoPtr) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u)) {
        return kResultInvalid;
    }

    try {
        if (Memory::Read8(fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset)) !=
            kOpenFlag) {
            return kResultInvalid;
        }

        const std::int32_t fd = static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        {
            std::lock_guard<std::mutex> lock(g_fileMutex);
            const auto it = g_fileHandles.find(fd);
            if (it != g_fileHandles.end()) {
                if (it->second.file && it->second.mode >= 2 && !FlushToDisk(it->second.file)) {
                    return kResultUnknown;
                }
                const bool closed = !it->second.file || std::fclose(it->second.file) == 0;
                // fclose consumes the stream even on error; never retain its pointer.
                g_fileHandles.erase(it);
                if (!closed) {
                    return kResultUnknown;
                }
            }
        }

        Memory::Write8(
            fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset), kClosedFlag);
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t SafeCloseSync(std::uint32_t fileInfoPtr) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u)) {
        return kResultInvalid;
    }

    try {
        const std::uint8_t openFlag =
            Memory::Read8(fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset));
        if (openFlag == kOpenFlag) {
            return CloseSync(fileInfoPtr);
        }
        if (openFlag != kSafeOpenFlag && openFlag != kSafeOpenAsyncFlag) {
            // Pinned WiiCompiled tolerates repeated safe-close after an async chain.
            return kResultOk;
        }

        const std::int32_t fd = static_cast<std::int32_t>(Memory::Read32(fileInfoPtr));
        // Keep ownership exclusive until the scratch is committed or removed.
        // Another safe opener must not replace this shadow during the commit.
        std::lock_guard<std::mutex> lock(g_fileMutex);
        const auto it = g_fileHandles.find(fd);
        if (it == g_fileHandles.end() || !it->second.file) {
            return kResultInvalid;
        }
        FileHandle handle = it->second;
        g_fileHandles.erase(it);

        bool ok = true;
        if (handle.mode >= 2) {
            ok = FlushToDisk(handle.file);
        }
        if (std::fclose(handle.file) != 0) {
            ok = false;
        }

        if (!ok) {
            if (!handle.safeCommitPath.empty()) {
                std::error_code cleanup;
                std::filesystem::remove(handle.path, cleanup);
            }
            return kResultUnknown;
        }

        if (!handle.safeCommitPath.empty()) {
            // The scratch file is created beside the target, keeping rename on the
            // same filesystem. Horizon/POSIX rename publishes the complete file in
            // one namespace operation, preserving the old target on pre-rename crash.
            std::error_code ec;
            std::filesystem::rename(handle.path, handle.safeCommitPath, ec);
            if (ec) {
                std::error_code cleanup;
                std::filesystem::remove(handle.path, cleanup);
                return kResultUnknown;
            }
#if !defined(_WIN32)
            // Match the pinned POSIX commit: sync the renamed directory entry too.
            const int directory = ::open(handle.safeCommitPath.parent_path().c_str(), O_RDONLY);
            if (directory >= 0) {
                ::fsync(directory);
                ::close(directory);
            }
#endif
        }

        Memory::Write8(
            fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset),
            openFlag == kSafeOpenFlag ? kSafeClosedFlag : kSafeClosedAsyncFlag);
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t SafeCloseReadSync(std::uint32_t fileInfoPtr) noexcept {
    return SafeCloseSync(fileInfoPtr);
}

std::int32_t CreateSync(std::uint32_t pathPtr,
                        std::uint32_t permissions,
                        std::uint32_t attributes) noexcept {
    (void)permissions;
    (void)attributes;

    try {
        std::filesystem::path hostPath;
        if (!ResolveHostPath(pathPtr, hostPath)) {
            return kResultInvalid;
        }

        std::error_code ec;
        if (std::filesystem::exists(hostPath, ec)) {
            return kResultExists;
        }
        if (ec) {
            return kResultUnknown;
        }

        std::filesystem::create_directories(hostPath.parent_path(), ec);
        if (ec) {
            return kResultUnknown;
        }

        FILE* file = std::fopen(hostPath.c_str(), "wbx");
        if (!file) {
            return errno == EEXIST ? kResultExists : kResultUnknown;
        }
        std::fclose(file); // Pinned NANDCreate ignores the close result.
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t DeleteSync(std::uint32_t pathPtr) noexcept {
    try {
        std::filesystem::path hostPath;
        if (!ResolveHostPath(pathPtr, hostPath)) {
            return kResultInvalid;
        }

        std::error_code ec;
        if (!std::filesystem::exists(hostPath, ec)) {
            return ec ? kResultUnknown : kResultNoExists;
        }

        const bool removed = std::filesystem::remove(hostPath, ec);
        if (ec || !removed) {
            return kResultUnknown;
        }
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t CreateDirSync(std::uint32_t pathPtr,
                           std::uint32_t permissions,
                           std::uint32_t attributes) noexcept {
    (void)permissions;
    (void)attributes;

    try {
        std::filesystem::path hostPath;
        if (!ResolveHostPath(pathPtr, hostPath)) {
            return kResultInvalid;
        }

        std::error_code ec;
        if (std::filesystem::exists(hostPath, ec)) {
            const bool directory = std::filesystem::is_directory(hostPath, ec);
            return ec ? kResultUnknown : directory ? kResultOk
                                                   : kResultExists;
        }
        if (ec) {
            return kResultUnknown;
        }

        const bool created = std::filesystem::create_directories(hostPath, ec);
        if (ec || !created) {
            return kResultUnknown;
        }
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

std::int32_t GetTypeSync(std::uint32_t pathPtr,
                         std::uint32_t outTypePtr) noexcept {
    if (pathPtr == 0u || outTypePtr == 0u || !Memory::Contains(outTypePtr, 1u)) {
        return kResultInvalid;
    }

    try {
        std::filesystem::path hostPath;
        if (!ResolveHostPath(pathPtr, hostPath)) {
            return kResultInvalid;
        }
        std::error_code ec;
        const bool exists = std::filesystem::exists(hostPath, ec);
        if (ec) {
            return kResultUnknown;
        }
        if (!exists) {
            return kResultNoExists;
        }

        const bool isDirectory = std::filesystem::is_directory(hostPath, ec);
        if (ec) {
            return kResultUnknown;
        }

        Memory::Write8(outTypePtr, isDirectory ? 2u : 1u);
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

} // namespace mkw::switch_nand_runtime
