#include "switch_nand_runtime.hpp"

#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace mkw::switch_nand_runtime {
namespace {

constexpr std::int32_t kResultOk = 0;
constexpr std::int32_t kResultInvalid = -8;
constexpr std::int32_t kResultNoExists = -12;
constexpr std::int32_t kResultUnknown = -64;
constexpr std::size_t kOpenFlagOffset = 0x8au;

struct FileHandle {
    FILE* file = nullptr;
    std::filesystem::path path;
    std::int32_t mode = 0;
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
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) {
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

} // namespace

std::int32_t OpenSync(std::uint32_t pathPtr,
                      std::uint32_t fileInfoPtr,
                      std::uint32_t mode) noexcept {
    if (fileInfoPtr == 0u || !Memory::Contains(fileInfoPtr, kOpenFlagOffset + 1u) ||
        (mode != 1u && mode != 2u && mode != 3u)) {
        return kResultInvalid;
    }

    try {
        std::string guestPath;
        if (!ReadGuestCString(pathPtr, guestPath)) {
            return kResultInvalid;
        }
        const std::filesystem::path hostPath = HostPath(Normalize(guestPath));

        FILE* file = nullptr;
        if (mode == 1u) {
            file = std::fopen(hostPath.c_str(), "rb");
        } else {
            file = std::fopen(hostPath.c_str(), "r+b");
            if (!file) {
                std::error_code ec;
                std::filesystem::create_directories(hostPath.parent_path(), ec);
                file = std::fopen(hostPath.c_str(), "w+b");
            }
        }
        if (!file) {
            return mode == 1u ? kResultNoExists : kResultUnknown;
        }

        std::int32_t fd = 0;
        {
            std::lock_guard<std::mutex> lock(g_fileMutex);
            fd = g_nextFd++;
            g_fileHandles.emplace(fd, FileHandle{file, hostPath, static_cast<std::int32_t>(mode)});
        }
        Memory::Write32(fileInfoPtr, static_cast<std::uint32_t>(fd));
        Memory::Write8(fileInfoPtr + static_cast<std::uint32_t>(kOpenFlagOffset), 1u);
        return kResultOk;
    } catch (...) {
        return kResultInvalid;
    }
}

} // namespace mkw::switch_nand_runtime
