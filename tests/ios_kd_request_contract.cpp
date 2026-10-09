#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t base = 0x70000000, path = base + 64, input = base + 256, output = base + 512;
std::vector<std::uint8_t> bytes;
bool active = false;
const char* stage = nullptr;
const char* expectedReason = nullptr;
std::uint32_t expectedTarget = 0;
CpuContext expectedCpu{};
std::vector<std::uint8_t> expectedMemory;
int pipeFd = -1;

CpuContext Cpu() {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    return cpu;
}
void ExpectedWord(std::vector<std::uint8_t>& expected, std::uint32_t address, std::uint32_t word) {
    for (unsigned i = 0; i < 4; ++i)
        expected[address - base + i] = word >> (24 - 8 * i);
}
void CheckCpu(const CpuContext& actual, CpuContext expected, std::uint32_t result) {
    expected.gpr[3] = result;
    assert(std::memcmp(&actual, &expected, sizeof(actual)) == 0);
}
std::uint32_t Open() {
    auto cpu = Cpu();
    cpu.gpr[3] = path;
    cpu.gpr[4] = 0;
    const auto before = cpu;
    const auto memory = bytes;
    KnownNativeCpuCall<0x801938F8u>::Invoke(&cpu);
    assert(cpu.gpr[3] >= 2000);
    CheckCpu(cpu, before, cpu.gpr[3]);
    assert(bytes == memory);
    assert(std::strcmp(stage, "RMCP01_IOS_OPEN_KD_REQUEST") == 0);
    return cpu.gpr[3];
}
CpuContext IoctlCpu(std::uint32_t fd, std::uint32_t cmd) {
    auto cpu = Cpu();
    cpu.gpr[3] = fd;
    cpu.gpr[4] = cmd;
    cpu.gpr[5] = cmd == 2 ? input : 0;
    cpu.gpr[6] = cmd == 2 ? 32 : 0;
    cpu.gpr[7] = output;
    cpu.gpr[8] = 32;
    return cpu;
}
void Ioctl(std::uint32_t fd, std::uint32_t cmd, std::uint32_t result) {
    // Distinct canaries in the input, untouched output tail and adjacent bytes.
    for (unsigned i = 0; i < 64; ++i)
        bytes[output - base + i] = 0x80 + i;
    auto cpu = IoctlCpu(fd, cmd);
    const auto before = cpu;
    auto expected = bytes;
    if (cmd == 15) {
        std::fill_n(expected.begin() + output - base, 32, 0);
        ExpectedWord(expected, output + 4, 1);
        ExpectedWord(expected, output + 8, 0x4d4b5752);
        ExpectedWord(expected, output + 12, 1);
    } else
        ExpectedWord(expected, output, result);
    KnownNativeCpuCall<0x80194290u>::Invoke(&cpu);
    CheckCpu(cpu, before, 0);
    assert(bytes == expected);
    assert(std::strcmp(stage, "RMCP01_IOS_IOCTL_KD_REQUEST") == 0);
}
void Close(std::uint32_t fd) {
    auto cpu = Cpu();
    cpu.gpr[3] = fd;
    const auto before = cpu;
    const auto memory = bytes;
    KnownNativeCpuCall<0x80193AD8u>::Invoke(&cpu);
    CheckCpu(cpu, before, 0);
    assert(bytes == memory);
    assert(std::strcmp(stage, "RMCP01_IOS_CLOSE_KD_REQUEST") == 0);
}
void Refusal(CpuContext cpu, std::uint32_t target, const char* reason) {
    expectedCpu = cpu;
    expectedMemory = bytes;
    expectedReason = reason;
    expectedTarget = target;
    int fds[2];
    assert(pipe(fds) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(fds[0]);
        pipeFd = fds[1];
        const rlimit noCore{0, 0};
        assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        switch (target) {
        case 0x801938f8:
            KnownNativeCpuCall<0x801938F8u>::Invoke(&cpu);
            break;
        case 0x80194290:
            KnownNativeCpuCall<0x80194290u>::Invoke(&cpu);
            break;
        case 0x80193ad8:
            KnownNativeCpuCall<0x80193AD8u>::Invoke(&cpu);
            break;
        default:
            std::abort();
        }
        _exit(9);
    }
    close(fds[1]);
    char proof = 0;
    assert(read(fds[0], &proof, 1) == 1 && proof == 'P');
    close(fds[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
} // namespace
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 4096);
    bytes.assign(4096, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == base ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(expectedReason && std::strcmp(reason, expectedReason) == 0 && target == expectedTarget);
    assert(cpu && std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0 && bytes == expectedMemory);
    assert(write(pipeFd, "P", 1) == 1);
}
int main(int argc, char**) {
    std::filesystem::create_directories("sdmc:/switch/WiiCompiled-Switch");
    Memory::Config config;
    config.regions = {{"synthetic-kd", base, 4096}};
    Memory::Init(config);
    constexpr char device[] = "/dev/net/kd/request";
    std::memcpy(bytes.data() + path - base, device, sizeof(device));
    if (argc > 1) {
        const auto fd = Open();
        Ioctl(fd, 3, 0);
        Ioctl(fd, 2, static_cast<std::uint32_t>(-42));
        Ioctl(fd, 2, static_cast<std::uint32_t>(-42));
        Ioctl(fd, 3, 0);
        Ioctl(fd, 2, static_cast<std::uint32_t>(-42));
        Ioctl(fd, 2, 0);
        Close(fd);
        std::puts("PASS: resume before first probe and repeated Boot probes preserve the pinned phase transition");
        return 0;
    }
    // Exact previously observed initialization, then the new fifth request.
    auto fd = Open();
    assert(fd == 2000);
    Ioctl(fd, 2, static_cast<std::uint32_t>(-42));
    Close(fd);
    fd = Open();
    assert(fd == 2001);
    Ioctl(fd, 1, 0);
    Close(fd);
    fd = Open();
    assert(fd == 2002);
    Ioctl(fd, 15, 0);
    Close(fd);
    fd = Open();
    assert(fd == 2003);
    Ioctl(fd, 3, 0);
    Close(fd);
    fd = Open();
    assert(fd == 2004);
    Ioctl(fd, 2, static_cast<std::uint32_t>(-42));
    Ioctl(fd, 2, 0);
    Ioctl(fd, 3, 0);
    Ioctl(fd, 2, 0);
    Close(fd);
    // Phase survives handle close/reopen and does not depend on an ordinal.
    for (unsigned i = 0; i < 96; ++i) {
        auto next = Open();
        assert(next == 2005 + i);
        Ioctl(next, 2, 0);
        Ioctl(next, 1, 0);
        Ioctl(next, 15, 0);
        Close(next);
    }
    const auto live = Open();
    auto invalid = IoctlCpu(fd, 2);
    Refusal(invalid, 0x80194290, "IOS_IOCTL_KD_INVALID_HANDLE");
    invalid = Cpu();
    invalid.gpr[3] = fd;
    Refusal(invalid, 0x80193ad8, "IOS_CLOSE_KD_INVALID_HANDLE");
    for (auto command : {0u, 4u, 6u, 7u, 0xffffffffu}) {
        invalid = IoctlCpu(live, command);
        Refusal(invalid, 0x80194290, "IOS_IOCTL_KD_UNPROVEN_COMMAND");
    }
    for (auto length : {0u, 4u, 31u, 33u, 0xffffffffu}) {
        invalid = IoctlCpu(live, 2);
        invalid.gpr[8] = length;
        Refusal(invalid, 0x80194290, "IOS_IOCTL_KD_UNPROVEN_ARGUMENTS");
    }
    for (auto address : {0u, base - 1, base + 4096 - 4, 0xfffffff0u}) {
        invalid = IoctlCpu(live, 2);
        invalid.gpr[7] = address;
        Refusal(invalid, 0x80194290, address ? "IOS_IOCTL_KD_INVALID_BUFFER" : "IOS_IOCTL_KD_UNPROVEN_ARGUMENTS");
        invalid = IoctlCpu(live, 2);
        invalid.gpr[5] = address;
        Refusal(invalid, 0x80194290, address ? "IOS_IOCTL_KD_INVALID_BUFFER" : "IOS_IOCTL_KD_UNPROVEN_ARGUMENTS");
    }
    invalid = IoctlCpu(live, 1);
    invalid.gpr[5] = input;
    invalid.gpr[6] = 32;
    Refusal(invalid, 0x80194290, "IOS_IOCTL_KD_UNPROVEN_ARGUMENTS");
    invalid = Cpu();
    invalid.gpr[3] = path;
    invalid.gpr[4] = 1;
    Refusal(invalid, 0x801938f8, "IOS_OPEN_UNPROVEN_PATH_OR_MODE");
    for (auto address : {0u, base - 1, base + 4096 - 1, 0xffffffffu}) {
        invalid = Cpu();
        invalid.gpr[3] = address;
        invalid.gpr[4] = 0;
        Refusal(invalid, 0x801938f8, "IOS_OPEN_PATH_UNREADABLE");
    }
    bytes[path - base + 5] = 'x';
    invalid = Cpu();
    invalid.gpr[3] = path;
    invalid.gpr[4] = 0;
    Refusal(invalid, 0x801938f8, "IOS_OPEN_UNPROVEN_PATH_OR_MODE");
    bytes[path - base + 5] = 'n';
    // Concurrent live handle bound; reuse table slots without reusing old IDs.
    std::vector<std::uint32_t> handles{live};
    for (unsigned i = 1; i < 32; ++i)
        handles.push_back(Open());
    invalid = Cpu();
    invalid.gpr[3] = path;
    invalid.gpr[4] = 0;
    Refusal(invalid, 0x801938f8, "IOS_OPEN_KD_HANDLE_LIMIT");
    Close(handles[7]);
    auto replacement = Open();
    assert(replacement > handles.back());
    for (auto handle : handles)
        if (handle != handles[7])
            Close(handle);
    Ioctl(replacement, 2, 0);
    Close(replacement);
    KnownNativeCpuCall<0x801938F8u>::Invoke(nullptr);
    KnownNativeCpuCall<0x80194290u>::Invoke(nullptr);
    KnownNativeCpuCall<0x80193AD8u>::Invoke(nullptr);
    Memory::Reset();
    std::puts("PASS: real KD traits/bridge preserve CPU and full guest ranges, phase replies and handle lifetime; unknown/malformed calls abort before mutation");
}
