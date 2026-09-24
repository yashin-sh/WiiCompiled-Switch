#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mkw::switch_ios_kd_hle {

constexpr std::uint32_t kIosOpenAddress = 0x801938F8u;
constexpr const char* kKdRequestPath = "/dev/net/kd/request";
constexpr std::uint32_t kFirstNetworkDeviceFd = 2000u;

inline bool ReadGuestCString(std::uint32_t address, char* out, std::size_t capacity) noexcept {
    if (!out || capacity == 0u || address == 0u || !Memory::IsInitialized()) {
        return false;
    }
    out[0] = '\0';

    for (std::size_t i = 0; i + 1u < capacity; ++i) {
        const std::uint32_t guest = address + static_cast<std::uint32_t>(i);
        if (!Memory::Contains(guest, 1u)) {
            out[i] = '\0';
            return false;
        }

        try {
            const char ch = static_cast<char>(Memory::Read8(guest));
            out[i] = ch;
            if (ch == '\0') {
                return true;
            }
        } catch (...) {
            out[i] = '\0';
            return false;
        }
    }

    out[capacity - 1u] = '\0';
    return false;
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
inline void WriteStatus(
    const char* status,
    const char* path,
    std::uint32_t mode,
    std::uint32_t fd) noexcept {
    constexpr const char* kStatusPath =
        "sdmc:/switch/WiiCompiled-Switch/fast-track-ios-open-kd-request.txt";
    FILE* out = std::fopen(kStatusPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\n"
        "path=%s\n"
        "mode=%u\n"
        "fd=%u\n",
        status ? status : "<null>",
        path ? path : "<null>",
        mode,
        fd);
    std::fclose(out);
}
#else
inline void WriteStatus(const char*, const char*, std::uint32_t, std::uint32_t) noexcept {}
#endif

[[noreturn]] inline void AbortBoundary(
    const char* status,
    CpuContext* cpu,
    const char* path,
    std::uint32_t mode) noexcept {
    WriteStatus(status, path, mode, 0u);
    mkw_switch_report_unsupported_translated_dispatch(
        status, kIosOpenAddress, cpu);
    std::abort();
}

inline std::uint32_t OpenKdRequest(CpuContext* cpu) noexcept {
    if (!cpu) {
        return 0u;
    }

    const std::uint32_t pathPtr = cpu->gpr[3];
    const std::uint32_t mode = cpu->gpr[4];

    char path[64]{};
    if (!ReadGuestCString(pathPtr, path, sizeof(path))) {
        AbortBoundary("IOS_OPEN_PATH_UNREADABLE", cpu, "<unreadable>", mode);
    }

    // Hardware has proven only this exact boot-time request. Keep the bridge
    // intentionally narrower than pinned Network_HLE_OpenDevice so any later
    // IOS path remains a fresh hardware-defined frontier.
    if (std::strcmp(path, kKdRequestPath) != 0 || mode != 0u) {
        AbortBoundary("IOS_OPEN_UNPROVEN_PATH_OR_MODE", cpu, path, mode);
    }

    // Pinned WiiCompiled enables networking by default and allocates network
    // device handles monotonically starting at 2000. No socket or host network
    // activity occurs merely from opening /dev/net/kd/request.
    static std::uint32_t nextDeviceFd = kFirstNetworkDeviceFd;
    const std::uint32_t fd = nextDeviceFd++;

    WriteStatus("open-pass", path, mode, fd);
    return fd;
}

} // namespace mkw::switch_ios_kd_hle

// IOS_Open / NAND_IOS_Open_HLE (PAL 0x801938F8). Hardware identifies the
// first live request exactly as "/dev/net/kd/request", mode 0. Mirror only the
// pinned device-allocation result here. IOCTL/IOCTLV/close and all neighboring
// IOS/network devices remain unsupported until hardware reaches them.
template <>
struct KnownNativeCpuCall<0x801938F8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = mkw::switch_ios_kd_hle::OpenKdRequest(cpu);
        }
    }
};
