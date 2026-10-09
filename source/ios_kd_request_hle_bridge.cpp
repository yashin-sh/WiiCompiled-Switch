#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
constexpr std::uint32_t kIosOpenAddress = 0x801938F8u;
constexpr std::uint32_t kIosIoctlAddress = 0x80194290u;
constexpr std::uint32_t kIosCloseAddress = 0x80193AD8u;
constexpr const char* kKdRequestPath = "/dev/net/kd/request";
constexpr std::uint32_t kReplyLength = 0x20u;
constexpr std::uint64_t kGeneratedUserId = 0x000000014D4B5752ull;
// Bound concurrent device bookkeeping without binding commands to an open
// ordinal. Closed slots are reusable; monotonically allocated IDs are not.
std::array<std::uint32_t, 32> gOpenHandles{};
std::uint32_t gNextHandle = 2000;
enum class TrySuspendPhase { Boot,
                             PostResumeProbe,
                             Ready };
TrySuspendPhase gPhase = TrySuspendPhase::Boot;
bool gBootProbeSeen = false;

bool ReadGuestCString(std::uint32_t address, char* out, std::size_t capacity) noexcept {
    if (!out || !capacity || !address || !Memory::IsInitialized())
        return false;
    out[0] = '\0';
    for (std::size_t i = 0; i + 1 < capacity; ++i) {
        if (i > std::numeric_limits<std::uint32_t>::max() - address)
            return false;
        const auto guest = address + static_cast<std::uint32_t>(i);
        if (!Memory::Contains(guest, 1))
            return false;
        try {
            out[i] = static_cast<char>(Memory::Read8(guest));
            if (out[i] == '\0')
                return true;
        } catch (...) {
            return false;
        }
    }
    out[capacity - 1] = '\0';
    return false;
}
std::uint32_t* FindHandle(std::uint32_t fd) noexcept {
    if (!fd)
        return nullptr;
    for (auto& handle : gOpenHandles)
        if (handle == fd)
            return &handle;
    return nullptr;
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
void WriteStatus(
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
void WriteStatus(const char*, const char*, std::uint32_t, std::uint32_t) noexcept {}
#endif

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
void WriteIoctlStatus(
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
void WriteIoctlStatus(
    const char*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t) noexcept {}
#endif

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
void WriteCloseStatus(const char* status, std::uint32_t fd) noexcept {
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
void WriteCloseStatus(const char*, std::uint32_t) noexcept {}
#endif

[[noreturn]] void AbortOpen(const char* status, CpuContext* cpu, const char* path, std::uint32_t mode) noexcept {
    WriteStatus(status, path, mode, 0);
    mkw_switch_report_unsupported_translated_dispatch(status, kIosOpenAddress, cpu);
    std::abort();
}
[[noreturn]] void AbortIoctl(const char* status, CpuContext* cpu) noexcept {
    WriteIoctlStatus(status, cpu->gpr[3], cpu->gpr[4], cpu->gpr[5], cpu->gpr[6], cpu->gpr[7], cpu->gpr[8]);
    mkw_switch_report_unsupported_translated_dispatch(status, kIosIoctlAddress, cpu);
    std::abort();
}
[[noreturn]] void AbortClose(const char* status, CpuContext* cpu) noexcept {
    WriteCloseStatus(status, cpu->gpr[3]);
    mkw_switch_report_unsupported_translated_dispatch(status, kIosCloseAddress, cpu);
    std::abort();
}
} // namespace

extern "C" void mkw_switch_hle_ios_open_kd_request(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_IOS_OPEN_KD_REQUEST");
    char path[64]{};
    const auto mode = cpu->gpr[4];
    if (!ReadGuestCString(cpu->gpr[3], path, sizeof(path)))
        AbortOpen("IOS_OPEN_PATH_UNREADABLE", cpu, "<unreadable>", mode);
    if (std::strcmp(path, kKdRequestPath) || mode != 0)
        AbortOpen("IOS_OPEN_UNPROVEN_PATH_OR_MODE", cpu, path, mode);
    std::uint32_t* slot = nullptr;
    for (auto& handle : gOpenHandles)
        if (!handle) {
            slot = &handle;
            break;
        }
    if (!slot || gNextHandle > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
        AbortOpen("IOS_OPEN_KD_HANDLE_LIMIT", cpu, path, mode);
    // Pinned Network_HLE_OpenDevice/AllocateDevice: opening this request node
    // allocates a handle and performs no host socket or network operation.
    const auto fd = gNextHandle++;
    *slot = fd;
    WriteStatus("open-pass", path, mode, fd);
    cpu->gpr[3] = fd;
}

extern "C" void mkw_switch_hle_ios_ioctl_kd_request(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_IOS_IOCTL_KD_REQUEST");
    const auto fd = cpu->gpr[3], cmd = cpu->gpr[4];
    const auto in = cpu->gpr[5], inLen = cpu->gpr[6];
    const auto out = cpu->gpr[7], outLen = cpu->gpr[8];
    if (!FindHandle(fd))
        AbortIoctl("IOS_IOCTL_KD_INVALID_HANDLE", cpu);
    // Only the already attributed request commands and observed buffer shapes
    // are admitted. Unknown commands (including socket startup) still stop.
    if (cmd != 1 && cmd != 2 && cmd != 3 && cmd != 0x0f)
        AbortIoctl("IOS_IOCTL_KD_UNPROVEN_COMMAND", cpu);
    if (out == 0 || outLen != kReplyLength ||
        (cmd == 2 ? (in == 0 || inLen != kReplyLength) : (in != 0 || inLen != 0)))
        AbortIoctl("IOS_IOCTL_KD_UNPROVEN_ARGUMENTS", cpu);
    if (!Memory::IsInitialized() || !Memory::Contains(out, outLen) ||
        (cmd == 2 && !Memory::Contains(in, inLen)))
        AbortIoctl("IOS_IOCTL_KD_INVALID_BUFFER", cpu);
    // Validate the complete declared ranges before touching CPU, guest bytes
    // or phase state. Handle lifetime and scheduler phase are independent.
    const char* status = nullptr;
    switch (cmd) {
    case 1:
        Memory::Write32(out, 0);
        status = "cmd1-suspend-pass";
        break;
    case 2:
        if (gPhase == TrySuspendPhase::Boot) {
            Memory::Write32(out, static_cast<std::uint32_t>(-42));
            gBootProbeSeen = true;
            status = "cmd2-boot-probe-pass";
        } else if (gPhase == TrySuspendPhase::PostResumeProbe) {
            // Pinned HandleKdIoctl deliberately preserves one pending reply
            // after resume. Returning zero here reuses cmd 3's success and
            // causes the guest SDK panic noted in the pinned implementation.
            Memory::Write32(out, static_cast<std::uint32_t>(-42));
            gPhase = TrySuspendPhase::Ready;
            status = "cmd2-post-resume-probe-pass";
        } else {
            Memory::Write32(out, 0);
            status = "cmd2-ready-pass";
        }
        break;
    case 3:
        Memory::Write32(out, 0);
        if (gBootProbeSeen && gPhase == TrySuspendPhase::Boot)
            gPhase = TrySuspendPhase::PostResumeProbe;
        status = "cmd3-resume-pass";
        break;
    case 0x0f:
        for (std::uint32_t i = 0; i < outLen; ++i)
            Memory::Write8(out + i, 0);
        Memory::Write64(out + 4, kGeneratedUserId);
        Memory::Write32(out + 0x0c, 1);
        status = "cmd15-user-id-pass";
        break;
    }
    WriteIoctlStatus(status, fd, cmd, in, inLen, out, outLen);
    cpu->gpr[3] = 0;
}

extern "C" void mkw_switch_hle_ios_close_kd_request(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_IOS_CLOSE_KD_REQUEST");
    const auto fd = cpu->gpr[3];
    auto* slot = FindHandle(fd);
    if (!slot)
        AbortClose("IOS_CLOSE_KD_INVALID_HANDLE", cpu);
    // Pinned Network_HLE_Close removes a live request handle and returns zero;
    // it does not reset the process-global KD scheduler phase.
    *slot = 0;
    WriteCloseStatus("close-pass", fd);
    cpu->gpr[3] = 0;
}
#endif
