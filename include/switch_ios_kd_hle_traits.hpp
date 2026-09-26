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
constexpr std::uint32_t kThirdNetworkDeviceFd = 2002u;
constexpr std::uint32_t kFourthNetworkDeviceFd = 2003u;
constexpr std::uint32_t kKdSuspendSchedulerCommand = 1u;
constexpr std::uint32_t kKdTrySuspendSchedulerCommand = 2u;
constexpr std::uint32_t kKdResumeSchedulerCommand = 3u;
constexpr std::uint32_t kKdRequestGeneratedUserIdCommand = 0x0Fu;
constexpr std::uint64_t kRuntimeGeneratedUserId = 0x000000014D4B5752ull;
constexpr std::uint32_t kKdBootProbeInputLength = 0x20u;
constexpr std::uint32_t kKdBootProbeOutputLength = 0x20u;

inline bool gFirstKdRequestOpen = false;
inline bool gFirstKdBootProbeSeen = false;
inline bool gSecondKdRequestOpen = false;
inline bool gSecondKdSuspendSeen = false;
inline bool gThirdKdRequestOpen = false;
inline bool gThirdKdGeneratedUserIdSeen = false;
inline bool gFourthKdRequestOpen = false;
inline bool gFourthKdResumeSeen = false;

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
    } else if (fd == kThirdNetworkDeviceFd) {
        gThirdKdRequestOpen = true;
    } else if (fd == kFourthNetworkDeviceFd) {
        gFourthKdRequestOpen = true;
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

    if (gFirstKdBootProbeSeen) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD2_REPEAT_UNPROVEN", cpu);
    }

    // Pinned WiiCompiled HandleKdIoctl(cmd=2) in Boot phase writes the WC24
    // result word -42 to outBuf and returns IOS result 0. Memory::Write32 uses
    // the guest big-endian representation, matching the pinned WriteReturn().
    Memory::Write32(outBuf, static_cast<std::uint32_t>(-42));
    gFirstKdBootProbeSeen = true;
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

    if (gSecondKdSuspendSeen) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD1_REPEAT_UNPROVEN", cpu);
    }

    // Pinned WiiCompiled HandleKdIoctl(cmd=1) writes WC24 result 0 at out+0
    // and returns IOS result 0. No socket or scheduler host side effect occurs.
    Memory::Write32(outBuf, 0u);
    gSecondKdSuspendSeen = true;
    WriteIoctlStatus("cmd1-second-request-suspend-pass", fd, cmd, inBuf, inLen, outBuf, outLen);
    cpu->gpr[3] = 0u;
}

inline void HandleThirdKdGeneratedUserId(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t fd = cpu->gpr[3];
    const std::uint32_t cmd = cpu->gpr[4];
    const std::uint32_t inBuf = cpu->gpr[5];
    const std::uint32_t inLen = cpu->gpr[6];
    const std::uint32_t outBuf = cpu->gpr[7];
    const std::uint32_t outLen = cpu->gpr[8];

    // Hardware proves exactly the third /dev/net/kd/request command-0x0F call:
    // fd 2002, no input buffer, and a 0x20-byte output buffer.
    if (fd != kThirdNetworkDeviceFd ||
        !gThirdKdRequestOpen ||
        cmd != kKdRequestGeneratedUserIdCommand ||
        inBuf != 0u ||
        inLen != 0u ||
        outBuf == 0u ||
        outLen != kKdBootProbeOutputLength) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD15_UNPROVEN_ARGUMENTS", cpu);
    }

    if (!Memory::IsInitialized() || !Memory::Contains(outBuf, outLen)) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD15_INVALID_BUFFER", cpu);
    }

    if (gThirdKdGeneratedUserIdSeen) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD15_REPEAT_UNPROVEN", cpu);
    }

    // Pinned WiiCompiled HandleKdIoctl(cmd=0x0F) zeroes the output buffer,
    // writes result 0, the stable generated user id, and creation stage 1.
    for (std::uint32_t offset = 0u; offset < outLen; ++offset) {
        Memory::Write8(outBuf + offset, 0u);
    }
    Memory::Write32(outBuf, 0u);
    Memory::Write64(outBuf + 4u, kRuntimeGeneratedUserId);
    Memory::Write32(outBuf + 0x0Cu, 1u);
    gThirdKdGeneratedUserIdSeen = true;
    WriteIoctlStatus("cmd15-third-request-user-id-pass", fd, cmd, inBuf, inLen, outBuf, outLen);
    cpu->gpr[3] = 0u;
}

inline void HandleFourthKdResumeScheduler(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t fd = cpu->gpr[3];
    const std::uint32_t cmd = cpu->gpr[4];
    const std::uint32_t inBuf = cpu->gpr[5];
    const std::uint32_t inLen = cpu->gpr[6];
    const std::uint32_t outBuf = cpu->gpr[7];
    const std::uint32_t outLen = cpu->gpr[8];

    // Hardware proves exactly the fourth /dev/net/kd/request command-3 call:
    // fd 2003, no input buffer, and a 0x20-byte output buffer after the
    // boot try-suspend -> suspend -> generated-user-id -> close sequence.
    if (fd != kFourthNetworkDeviceFd ||
        !gFourthKdRequestOpen ||
        !gFirstKdBootProbeSeen ||
        gThirdKdRequestOpen ||
        !gThirdKdGeneratedUserIdSeen ||
        cmd != kKdResumeSchedulerCommand ||
        inBuf != 0u ||
        inLen != 0u ||
        outBuf == 0u ||
        outLen != kKdBootProbeOutputLength) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD3_UNPROVEN_ARGUMENTS", cpu);
    }

    if (!Memory::IsInitialized() || !Memory::Contains(outBuf, 4u)) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD3_INVALID_BUFFER", cpu);
    }

    if (gFourthKdResumeSeen) {
        AbortIoctlBoundary("IOS_IOCTL_KD_CMD3_REPEAT_UNPROVEN", cpu);
    }

    // Pinned WiiCompiled HandleKdIoctl(cmd=3) advances the boot scheduler
    // phase from Boot to PostResumeProbe when the first try-suspend was seen,
    // writes WC24 result 0 at out+0, and returns IOS result 0. The local
    // resume-seen state records that exact transition for any later hardware-
    // proven post-resume probe without pre-porting that later command here.
    Memory::Write32(outBuf, 0u);
    gFourthKdResumeSeen = true;
    WriteIoctlStatus("cmd3-fourth-request-resume-pass", fd, cmd, inBuf, inLen, outBuf, outLen);
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

    if (cpu->gpr[3] == kThirdNetworkDeviceFd &&
        cpu->gpr[4] == kKdRequestGeneratedUserIdCommand) {
        HandleThirdKdGeneratedUserId(cpu);
        return;
    }

    if (cpu->gpr[3] == kFourthNetworkDeviceFd &&
        cpu->gpr[4] == kKdResumeSchedulerCommand) {
        HandleFourthKdResumeScheduler(cpu);
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

inline void CloseObservedKdRequest(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t fd = cpu->gpr[3];

    if (fd == kFirstNetworkDeviceFd && gFirstKdRequestOpen) {
        // Hardware-proven first KD request close.
        gFirstKdRequestOpen = false;
        WriteCloseStatus("close-first-pass", fd);
        cpu->gpr[3] = 0u;
        return;
    }

    if (fd == kSecondNetworkDeviceFd &&
        gSecondKdRequestOpen &&
        gSecondKdSuspendSeen) {
        // Hardware-proven second KD request close immediately after cmd=1.
        // Pinned Network_HLE_Close removes the valid network device handle and
        // returns IOS result 0 without any additional guest-memory mutation.
        gSecondKdRequestOpen = false;
        WriteCloseStatus("close-second-pass", fd);
        cpu->gpr[3] = 0u;
        return;
    }

    if (fd == kThirdNetworkDeviceFd &&
        gThirdKdRequestOpen &&
        gThirdKdGeneratedUserIdSeen) {
        // Hardware-proven third KD request close immediately after cmd=0x0F.
        // Pinned Network_HLE_Close removes the valid network device handle and
        // returns IOS result 0 without any additional guest-memory mutation.
        gThirdKdRequestOpen = false;
        WriteCloseStatus("close-third-pass", fd);
        cpu->gpr[3] = 0u;
        return;
    }

    AbortCloseBoundary("IOS_CLOSE_KD_UNPROVEN_FD", cpu);
}

} // namespace mkw::switch_ios_kd_hle

// IOS_Open / NAND_IOS_Open_HLE (PAL 0x801938F8). Hardware identifies the
// first live request exactly as "/dev/net/kd/request", mode 0. Mirror only the
// pinned device-allocation result here. The specializations below cover only
// the hardware-proven KD ioctl/close sequences through fd 2003; ioctlv and
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
// captured the fd-2000 command-2 Boot probe, fd-2001 command-1 suspend call,
// fd-2002 command-0x0F generated-user-id request, and fd-2003 command-3
// resume-scheduler call. Mirror only those exact live tuples. Repeated/variant
// calls, later command-2 phases, ioctlv and other network services remain
// unsupported until hardware reaches them.
template <>
struct KnownNativeCpuCall<0x80194290u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw::switch_ios_kd_hle::HandleObservedKdIoctl(cpu);
    }
};

// IOS_Close / NAND_IOS_Close_HLE (PAL 0x80193AD8). Hardware has proven the
// fd-2000 close after the first command-2 Boot probe, the fd-2001 close after
// command-1 suspend, and the fd-2002 close after command-0x0F generated-user-id.
// Mirror only those exact live handles/sequences.
template <>
struct KnownNativeCpuCall<0x80193AD8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw::switch_ios_kd_hle::CloseObservedKdRequest(cpu);
    }
};
