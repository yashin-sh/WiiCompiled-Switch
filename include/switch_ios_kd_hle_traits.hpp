#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mkw::switch_ios_kd_hle {

constexpr std::uint32_t kIosOpenAddress = 0x801938F8u;
constexpr std::uint32_t kIosCloseAddress = 0x80193AD8u;
constexpr std::uint32_t kIosIoctlAddress = 0x80194290u;
constexpr const char* kKdRequestPath = "/dev/net/kd/request";
constexpr std::uint32_t kFirstNetworkDeviceFd = 2000u;
constexpr std::uint32_t kSecondNetworkDeviceFd = 2001u;
constexpr std::uint32_t kKdSuspendSchedulerCommand = 1u;
constexpr std::uint32_t kKdTrySuspendSchedulerCommand = 2u;
constexpr std::uint32_t kKdBootProbeInputLength = 0x20u;
constexpr std::uint32_t kKdBootProbeOutputLength = 0x20u;

inline bool gFirstKdRequestOpen = false;
inline bool gSecondKdRequestOpen = false;

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

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
inline void WriteIoctlStatus(
    const char* status,
    std::uint32_t fd,
    std::uint32_t cmd,
    std::uint32_t inBuf,
    std::uint32_t inLen,
    std::uint32_t outBuf,
    std::uint32_t outLen) noexcept {
    constexpr const char* kStatusPath =
        "sdmc:/switch/WiiCompiled-Switch/fast-track-ios-ioctl-kd-cmd2.txt";
    FILE* out = std::fopen(kStatusPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\n"
        "fd=%u\n"
        "cmd=%u\n"
        "in=0x%08x/0x%08x\n"
        "out=0x%08x/0x%08x\n",
        status ? status : "<null>",
        fd,
        cmd,
        inBuf,
        inLen,
        outBuf,
        outLen);
    std::fclose(out);
}
#else
inline void WriteIoctlStatus(
    const char*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t) noexcept {}
#endif

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
inline void WriteCloseStatus(const char* status, std::uint32_t fd) noexcept {
    constexpr const char* kStatusPath =
        "sdmc:/switch/WiiCompiled-Switch/fast-track-ios-close-kd-request.txt";
    FILE* out = std::fopen(kStatusPath, "w");
    if (!out) {
        return;
    }
    std::fprintf(
        out,
        "status=%s\n"
        "fd=%u\n",
        status ? status : "<null>",
        fd);
    std::fclose(out);
}
#else
inline void WriteCloseStatus(const char*, std::uint32_t) noexcept {}
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
    if (fd == kFirstNetworkDeviceFd) {
        gFirstKdRequestOpen = true;
    } else if (fd == kSecondNetworkDeviceFd) {
        gSecondKdRequestOpen = true;
    }

    WriteStatus("open-pass", path, mode, fd);
    return fd;
}

[[noreturn]] inline void AbortIoctlBoundary(const char* status, CpuContext* cpu) noexcept {
    if (cpu) {
        WriteIoctlStatus(
            status,
            cpu->gpr[3],
            cpu->gpr[4],
            cpu->gpr[5],
            cpu->gpr[6],
            cpu->gpr[7],
            cpu->gpr[8]);
    }
    mkw_switch_report_unsupported_translated_dispatch(status, kIosIoctlAddress, cpu);
    std::abort();
}

inline void HandleFirstKdTrySuspend(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t fd = cpu->gpr[3];
    const std::uint32_t cmd = cpu->gpr[4];
    const std::uint32_t inBuf = cpu->gpr[5];
    const std::uint32_t inLen = cpu->gpr[6];
    const std::uint32_t outBuf = cpu->gpr[7];
    const std::uint32_t outLen = cpu->gpr[8];

    // Hardware proves exactly the first /dev/net/kd/request command-2 probe:
    // fd 2000, 0x20-byte input, and 0x20-byte output. Keep later KD commands
    // and later command-2 phases as fresh hardware-defined frontiers.
    if (fd != kFirstNetworkDeviceFd ||
        !gFirstKdRequestOpen ||
        cmd != kKdTrySuspendSchedulerCommand ||
        inLen != kKdBootProbeInputLength ||
        outLen != kKdBootProbeOutputLength ||
        inBuf == 0u ||
        outBuf == 0u) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD2_UNPROVEN_ARGUMENTS", cpu);
    }

    if (!Memory::IsInitialized() ||
        !Memory::Contains(inBuf, inLen) ||
        !Memory::Contains(outBuf, 4u)) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD2_INVALID_BUFFER", cpu);
    }

    static bool bootProbeSeen = false;
    if (bootProbeSeen) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD2_REPEAT_UNPROVEN", cpu);
    }

    // Pinned WiiCompiled HandleKdIoctl(cmd=2) in Boot phase writes the WC24
    // result word -42 to outBuf and returns IOS result 0. Memory::Write32 uses
    // the guest big-endian representation, matching the pinned WriteReturn().
    Memory::Write32(outBuf, static_cast<std::uint32_t>(-42));
    bootProbeSeen = true;
    WriteIoctlStatus("cmd2-boot-probe-pass", fd, cmd, inBuf, inLen, outBuf, outLen);
    cpu->gpr[3] = 0u;
}

inline void HandleSecondKdSuspendScheduler(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t fd = cpu->gpr[3];
    const std::uint32_t cmd = cpu->gpr[4];
    const std::uint32_t inBuf = cpu->gpr[5];
    const std::uint32_t inLen = cpu->gpr[6];
    const std::uint32_t outBuf = cpu->gpr[7];
    const std::uint32_t outLen = cpu->gpr[8];

    // Hardware proves exactly the second /dev/net/kd/request command-1 call:
    // fd 2001, no input buffer, and a 0x20-byte output buffer.
    if (fd != kSecondNetworkDeviceFd ||
        !gSecondKdRequestOpen ||
        cmd != kKdSuspendSchedulerCommand ||
        inBuf != 0u ||
        inLen != 0u ||
        outBuf == 0u ||
        outLen != kKdBootProbeOutputLength) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD1_UNPROVEN_ARGUMENTS", cpu);
    }

    if (!Memory::IsInitialized() || !Memory::Contains(outBuf, 4u)) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD1_INVALID_BUFFER", cpu);
    }

    static bool secondSuspendSeen = false;
    if (secondSuspendSeen) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD1_REPEAT_UNPROVEN", cpu);
    }

    // Pinned WiiCompiled HandleKdIoctl(cmd=1) writes WC24 result 0 at out+0
    // and returns IOS result 0. No socket or scheduler host side effect occurs.
    Memory::Write32(outBuf, 0u);
    secondSuspendSeen = true;
    WriteIoctlStatus("cmd1-second-request-suspend-pass", fd, cmd, inBuf, inLen, outBuf, outLen);
    cpu->gpr[3] = 0u;
}

inline void HandleObservedKdIoctl(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    if (cpu->gpr[3] == kFirstNetworkDeviceFd &&
        cpu->gpr[4] == kKdTrySuspendSchedulerCommand) {
        HandleFirstKdTrySuspend(cpu);
        return;
    }

    if (cpu->gpr[3] == kSecondNetworkDeviceFd &&
        cpu->gpr[4] == kKdSuspendSchedulerCommand) {
        HandleSecondKdSuspendScheduler(cpu);
        return;
    }

    AbortIoctlBoundary("IOS_IOCTL_KD_UNPROVEN_TUPLE", cpu);
}

[[noreturn]] inline void AbortCloseBoundary(const char* status, CpuContext* cpu) noexcept {
    if (cpu) {
        WriteCloseStatus(status, cpu->gpr[3]);
    }
    mkw_switch_report_unsupported_translated_dispatch(status, kIosCloseAddress, cpu);
    std::abort();
}

inline void CloseFirstKdRequest(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t fd = cpu->gpr[3];

    // Hardware proves only the first KD request handle returned by IOS_Open.
    // Pinned Network_HLE_Close removes that handle and returns IOS result 0.
    if (fd != kFirstNetworkDeviceFd || !gFirstKdRequestOpen) {
        AbortCloseBoundary("IOS_CLOSE_KD_UNPROVEN_FD", cpu);
    }

    gFirstKdRequestOpen = false;
    WriteCloseStatus("close-pass", fd);
    cpu->gpr[3] = 0u;
}

} // namespace mkw::switch_ios_kd_hle

// IOS_Open / NAND_IOS_Open_HLE (PAL 0x801938F8). Hardware identifies the
// first live request exactly as "/dev/net/kd/request", mode 0. Mirror only the
// pinned device-allocation result here. The specializations below cover only
// the first proven KD command-2 ioctl and the exact fd-2000 close; ioctlv and
// neighboring IOS/network devices remain unsupported until hardware reaches
// them.
template <>
struct KnownNativeCpuCall<0x801938F8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = mkw::switch_ios_kd_hle::OpenKdRequest(cpu);
        }
    }
};

// IOS_Ioctl / NAND_IOS_Ioctl_Entry_HLE (PAL 0x80194290). Hardware has
// captured the first fd-2000 command-2 Boot probe and the later fd-2001
// command-1 suspend-scheduler call. Mirror only those exact live tuples.
// Repeated/variant calls, command 3, ioctlv and other network services remain
// unsupported until hardware reaches them.
template <>
struct KnownNativeCpuCall<0x80194290u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw::switch_ios_kd_hle::HandleObservedKdIoctl(cpu);
    }
};

// IOS_Close / NAND_IOS_Close_HLE (PAL 0x80193AD8). Hardware reaches this
// boundary only after the first KD command-2 Boot probe and passes r3=2000.
// Mirror only the pinned Network_HLE_Close result for that exact live handle.
template <>
struct KnownNativeCpuCall<0x80193AD8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw::switch_ios_kd_hle::CloseFirstKdRequest(cpu);
    }
};
