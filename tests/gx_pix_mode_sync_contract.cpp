#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "gx_internal.h"
#include "switch_gx_hle_traits.hpp"

#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
struct Region {
    std::uint32_t base;
    std::size_t size, mappedSize;
    std::uint8_t* mapping;
    std::uint8_t* bytes;
};
std::vector<Region> regions;
std::vector<std::vector<std::uint8_t>> expectedMemory;
std::vector<char> calls;
CpuContext cpu{}, savedCpu{};
unsigned stages = 0, nativeCalls = 0, cases = 0;
bool observing = false, throwNative = false;
int reportPipe = -1;
unsigned reports = 0;
const char* reason = nullptr;
const char* stage = nullptr;
using Trait = KnownNativeCpuCall<0x8016EB70u>;
static_assert(Trait::kAvailable);
static_assert(noexcept(Trait::Invoke(nullptr)));

void CheckCpu() {
    assert(std::memcmp(&cpu, &savedCpu, sizeof(cpu)) == 0);
    assert(stage && std::strcmp(stage, "RMCP01_GX_PIX_MODE_SYNC") == 0);
}
void CheckMemory() {
    assert(expectedMemory.size() == regions.size());
    for (std::size_t i = 0; i < regions.size(); ++i)
        assert(std::memcmp(regions[i].bytes, expectedMemory[i].data(), regions[i].size) == 0);
}
void Prepare(std::uint32_t gd, std::size_t dataBytes = 16u, bool pointer = true) {
    observing = false;
    Memory::Config config;
    if (pointer)
        config.regions.push_back({"gx-pointer", kGXDataPtrAddr, 4u});
    if (dataBytes)
        config.regions.push_back({"gx-data", gd == 0xfffffffeu ? 0u : gd, dataBytes});
    Memory::Init(config);
    if (pointer)
        Memory::Write32(kGXDataPtrAddr, gd);
}
void Capture(std::uint32_t gd, bool pointer) {
    calls.clear();
    expectedMemory.clear();
    for (const auto& r : regions)
        expectedMemory.emplace_back(r.bytes, r.bytes + r.size);
    const std::uint32_t address = gd + 2u;
    if (pointer && gd && Memory::Contains(address, 2u)) {
        for (std::size_t i = 0; i < regions.size(); ++i) {
            const auto& r = regions[i];
            if (address >= r.base && std::uint64_t(address) + 2u <= std::uint64_t(r.base) + r.size) {
                expectedMemory[i][address - r.base] = 0;
                expectedMemory[i][address - r.base + 1u] = 0;
            }
        }
    }
    std::memcpy(&savedCpu, &cpu, sizeof(cpu));
    observing = true;
}
[[maybe_unused]] void Invoke(std::uint32_t gd, bool pointer = true) {
    Capture(gd, pointer);
    const auto oldStages = stages, oldNative = nativeCalls;
    Trait::Invoke(&cpu);
    CheckCpu();
    CheckMemory();
    assert(stages == oldStages + 1u && nativeCalls == oldNative + 1u);
    assert(calls == (pointer && gd ? std::vector<char>{'R', 'W', 'N'} : std::vector<char>{'R', 'N'}));
    observing = false;
    ++cases;
}
void CheckNull() {
    const auto oldStages = stages, oldNative = nativeCalls;
    const auto oldCalls = calls;
    Trait::Invoke(nullptr);
    assert(stages == oldStages && nativeCalls == oldNative && calls == oldCalls);
}
void Refusal(const char* expectedReason, bool nativeException, bool initialize = true) {
    if (initialize)
        Prepare(0x70002000u);
    else
        Memory::Reset();
    Capture(0x70002000u, true);
    int pipefd[2];
    assert(pipe(pipefd) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(pipefd[0]);
        reportPipe = pipefd[1];
        reason = expectedReason;
        throwNative = nativeException;
        reports = 0;
        Trait::Invoke(&cpu);
        _exit(90);
    }
    close(pipefd[1]);
    std::array<char, 3> proof{};
    std::size_t bytes = 0;
    for (;;) {
        const auto count = read(pipefd[0], proof.data() + bytes, proof.size() - bytes);
        assert(count >= 0);
        if (!count)
            break;
        bytes += static_cast<std::size_t>(count);
        assert(bytes < proof.size());
    }
    close(pipefd[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(bytes == 2u && proof[0] == 'R' && proof[1] == 'A');
    observing = false;
}
} // namespace

namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Shutdown() noexcept {
    for (const auto& r : regions)
        assert(munmap(r.mapping, r.mappedSize) == 0);
    regions.clear();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    Shutdown();
    const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    for (const auto& r : requests) {
        assert(r.size <= page);
        auto* mapping = static_cast<std::uint8_t*>(mmap(nullptr, 3u * page, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        assert(mapping != MAP_FAILED);
        assert(mprotect(mapping + page, page, PROT_READ | PROT_WRITE) == 0);
        auto* bytes = mapping + 2u * page - r.size;
        std::memset(bytes, 0xa5, r.size);
        regions.push_back({r.base, r.size, 3u * page, mapping, bytes});
    }
}
std::uint8_t* HostPointer(std::uint32_t address) {
    for (const auto& r : regions)
        if (address >= r.base && std::uint64_t(address) < std::uint64_t(r.base) + r.size)
            return r.bytes + (address - r.base);
    return nullptr;
}
} // namespace GuestFlat

extern "C" std::uint32_t __real__ZN6Memory6Read32Ej(std::uint32_t);
extern "C" void __real__ZN6Memory7Write16Ejt(std::uint32_t, std::uint16_t);
extern "C" std::uint32_t __wrap__ZN6Memory6Read32Ej(std::uint32_t address) {
    if (observing) {
        CheckCpu();
        assert(calls.empty() && address == kGXDataPtrAddr);
        calls.push_back('R');
    }
    return __real__ZN6Memory6Read32Ej(address);
}
extern "C" void __wrap__ZN6Memory7Write16Ejt(std::uint32_t address, std::uint16_t value) {
    if (observing) {
        CheckCpu();
        assert(calls == std::vector<char>{'R'} && value == 0u);
        calls.push_back('W');
    }
    __real__ZN6Memory7Write16Ejt(address, value);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stages;
}
extern "C" void GXPixModeSync() {
    CheckCpu();
    CheckMemory();
    assert(calls == std::vector<char>{'R'} || calls == (std::vector<char>{'R', 'W'}));
    calls.push_back('N');
    ++nativeCalls;
    if (throwNative)
        throw std::runtime_error("native transport fixture");
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* value, std::uint32_t target, CpuContext* context) noexcept {
    assert(reportPipe >= 0 && reports++ == 0u);
    assert(context == &cpu && target == 0x8016EB70u && std::strcmp(value, reason) == 0);
    CheckCpu();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    CheckMemory();
    assert(calls == (std::vector<char>{'R', 'W', 'N'}));
#else
    assert(calls.empty());
#endif
    assert(write(reportPipe, "R", 1) == 1);
}
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort() {
    if (reportPipe >= 0) {
        assert(reports == 1u);
        CheckCpu();
        assert(write(reportPipe, "A", 1) == 1);
    }
    __real_abort();
}

int main() {
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x5bu + 37u * i);
    CheckNull();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    constexpr std::uint32_t gd = 0x70002000u;
    Prepare(gd);
    for (unsigned value = 0; value < 65536u; ++value) {
        Memory::Write16(gd + 2u, static_cast<std::uint16_t>(value));
        Invoke(gd);
    }
    for (const auto base : {gd, gd + 1u, 0xfffffffeu, 0u})
        for (const auto size : {1u, 2u, 3u, 4u, 16u}) {
            Prepare(base, size);
            Invoke(base);
        }
    Prepare(gd, 0);
    Invoke(gd);
    Prepare(gd, 16u, false);
    Invoke(gd, false);
    Memory::Reset();
    Invoke(gd, false);
    Refusal("GX_PIX_MODE_SYNC_NATIVE_EXCEPTION", true);
    std::printf("PASS: PixModeSync rendered returns=%u diagnosed native refusal=1\n", cases);
#else
    Refusal("GX_PIX_MODE_SYNC_REQUIRES_RENDERER", false);
    Refusal("GX_PIX_MODE_SYNC_REQUIRES_RENDERER", false, false);
    std::puts("PASS: PixModeSync headless diagnosed refusals=2");
#endif
    Memory::Reset();
}
