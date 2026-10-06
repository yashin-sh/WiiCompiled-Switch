#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "gx_internal.h"
#include "switch_gx_hle_traits.hpp"
#include "switch_texture_copy_lifetime.hpp"

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

TexCopyState g_texCopyState;
namespace {
constexpr std::uint32_t base = 0x90010000u, size = 65536u;
constexpr std::array aliases{base, 0x10010000u, 0xd0010000u};
enum Event { Ensure,
             Snapshot,
             Drain,
             Source,
             Copy,
             Destroy };
std::vector<Event> events;
std::vector<void*> destroyed;
std::vector<std::pair<void*, std::size_t>> mappings;
std::array<std::uint8_t*, 3> backing{};
CpuContext expectedCpu{}, *liveCpu = nullptr;
const char *stage = "", *reason = nullptr;
unsigned valid = 0, refusals = 0, opens = 0;
bool active = true, initialized = true, changeConfig = false, throwCopy = false;
int reportFd = -1;
bool keepMapping = false, reenterDestroy = false;
void* expectedDestination = nullptr;

void CheckCpu() {
    if (liveCpu)
        assert(std::memcmp(liveCpu, &expectedCpu, sizeof(expectedCpu)) == 0);
}
void Note(Event event) {
    CheckCpu();
    events.push_back(event);
}
void Configure() {
    g_texCopyState = {0, 0, 128, 128, 128, 128, 5, 0};
}
CpuContext Cpu(std::uint32_t address, std::uint32_t count) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(i * 37u + 19u);
    cpu.gpr[3] = address;
    cpu.gpr[4] = count;
    return cpu;
}
void Prepare(CpuContext& cpu) {
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    liveCpu = &cpu;
    events.clear();
    destroyed.clear();
}
void CheckBytes() {
    for (auto* data : backing)
        for (std::size_t i = 0; i < size; ++i)
            assert(data[i] == 0xa5);
}
[[maybe_unused]] void CopyAt(std::uint32_t offset) {
    auto cpu = Cpu(base + offset, 1);
    Prepare(cpu);
    expectedDestination = backing[0] + offset;
    const auto shadow = g_texCopyState;
    KnownNativeCpuCall<0x8016FD74u>::Invoke(&cpu);
    CheckCpu();
    assert(std::strcmp(stage, "RMCP01_GX_COPY_TEX") == 0);
    assert((events == std::vector<Event>{Ensure, Snapshot, Drain, Source, Copy, Source}));
    assert(std::memcmp(&g_texCopyState, &shadow, sizeof(shadow)) == 0);
    liveCpu = nullptr;
    ++valid;
}
[[maybe_unused]] void Evict(std::uint32_t address, std::uint32_t count, std::vector<void*> expected) {
    events.clear();
    destroyed.clear();
    mkw_switch_gx_invalidate_copy_destinations(address, count);
    assert(destroyed == expected);
    assert(events == std::vector<Event>(expected.size(), Destroy));
    ++valid;
}
[[maybe_unused]] void Cache(void (*invoke)(CpuContext*) noexcept, std::uint32_t address,
                            std::uint32_t count, std::vector<void*> expected) {
    auto cpu = Cpu(address, count);
    Prepare(cpu);
    invoke(&cpu);
    CheckCpu();
    assert(destroyed == expected);
    assert(events == std::vector<Event>(expected.size(), Destroy));
    liveCpu = nullptr;
    ++valid;
}
void Refusal(std::uint32_t address, std::uint32_t clear, const char* expectedReason) {
    int pipeFd[2];
    assert(pipe(pipeFd) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(pipeFd[0]);
        reportFd = pipeFd[1];
        prctl(PR_SET_DUMPABLE, 0);
        auto cpu = Cpu(address, clear);
        Prepare(cpu);
        reason = expectedReason;
        KnownNativeCpuCall<0x8016FD74u>::Invoke(&cpu);
        _exit(90);
    }
    close(pipeFd[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    char proof[3]{};
    assert(read(pipeFd[0], proof, sizeof(proof)) == 2);
    close(pipeFd[0]);
    assert(proof[0] == 'R' && proof[1] == 'A');
    CheckBytes();
    ++refusals;
}
std::vector<Event> refusalPrefix;
} // namespace

namespace GuestFlat {
bool IsActive() {
    return !mappings.empty();
}
void Shutdown() noexcept {
    if (keepMapping)
        return;
    for (const auto& [pointer, bytes] : mappings)
        assert(munmap(pointer, bytes) == 0);
    mappings.clear();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    Shutdown();
    const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    assert(requests.size() == 3);
    for (std::size_t i = 0; i < requests.size(); ++i) {
        assert(requests[i].base == aliases[i] && requests[i].size == size);
        auto* ptr = static_cast<std::uint8_t*>(mmap(nullptr, size + 2 * page, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        assert(ptr != MAP_FAILED);
        assert(mprotect(ptr + page, size, PROT_READ | PROT_WRITE) == 0);
        backing[i] = ptr + page;
        std::memset(backing[i], 0xa5, size);
        mappings.emplace_back(ptr, size + 2 * page);
    }
}
std::uint8_t* HostPointer(std::uint32_t address) {
    for (std::size_t i = 0; i < aliases.size(); ++i)
        if (address >= aliases[i] && std::uint64_t(address) < std::uint64_t(aliases[i]) + size)
            return backing[i] + address - aliases[i];
    return nullptr;
}
} // namespace GuestFlat

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* value, std::uint32_t target,
                                                                  CpuContext*) noexcept {
    CheckCpu();
    CheckBytes();
    assert(target == 0x8016FD74u && std::strcmp(value, reason) == 0);
    assert(std::strcmp(stage, "RMCP01_GX_COPY_TEX") == 0);
    assert(events == refusalPrefix);
    assert(reportFd >= 0 && write(reportFd, "R", 1) == 1);
}
extern "C" [[noreturn]] void __wrap_abort() {
    CheckCpu();
    CheckBytes();
    assert(reportFd >= 0 && write(reportFd, "A", 1) == 1);
    std::signal(SIGABRT, SIG_DFL);
    std::raise(SIGABRT);
    _exit(91);
}
extern "C" FILE* __wrap_fopen(const char* path, const char* mode) {
    CheckCpu();
    assert(std::strcmp(path, "sdmc:/switch/WiiCompiled-Switch/fast-track-gx-copy-tex.txt") == 0);
    assert(std::strcmp(mode, "w") == 0);
    static char buffer[1024];
    ++opens;
    return fmemopen(buffer, sizeof(buffer), "w");
}
void EnsureAuroraFrameActive() {
    Note(Ensure);
}
extern "C" bool mkw_switch_renderer_has_active_frame() noexcept {
    Note(Snapshot);
    return initialized && active;
}
extern "C" void GXDrawDone() {
    Note(Drain);
    if (changeConfig)
        g_texCopyState.dstWidth = 64;
}
extern "C" void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height) {
    Note(Source);
    assert(left == 0 && top == 0 && width == 128 && height == 128);
}
extern "C" void GXCopyTex(void* destination, GXBool clear) {
    Note(Copy);
    assert(clear && destination == expectedDestination);
    if (throwCopy)
        throw std::runtime_error("native copy fault");
}
extern "C" void GXDestroyCopyTex(void* destination) {
    Note(Destroy);
    destroyed.push_back(destination);
    if (reenterDestroy) {
        reenterDestroy = false;
        mkw_switch_gx_invalidate_copy_destinations(base, size);
    }
}

int main() {
    Memory::Config config;
    for (auto a : aliases)
        config.regions.push_back({"test", a, size});
    Memory::Init(config);
    Configure();
    mkw_switch_hle_gx_copy_tex(nullptr);
    mkw_switch_hle_dc_range(nullptr);
    assert(events.empty());
#if MKW_LOCAL_RENDERED_FAST_TRACK
    // Every complete unaligned 32 KiB window; canonical aliases retire the
    // exact cached host pointer, not a differently mapped physical VA.
    for (std::uint32_t offset = 0; offset <= size - 32768u; ++offset) {
        CopyAt(offset);
        Evict(aliases[1] + offset, 32768u, {backing[0] + offset});
    }
    CopyAt(0);
    const auto previousOpens = opens;
    CopyAt(0);
    assert(opens == previousOpens);
    Evict(base + 32768u, 1, {});
    Evict(base, 0, {});
    Evict(base + 32767u, 1, {backing[0]});
    Evict(base, 32768u, {});
    const std::array cacheFunctions{KnownNativeCpuCall<0x801A1600u>::Invoke,
                                    KnownNativeCpuCall<0x801A162Cu>::Invoke,
                                    KnownNativeCpuCall<0x801A165Cu>::Invoke,
                                    KnownNativeCpuCall<0x801A168Cu>::Invoke,
                                    KnownNativeCpuCall<0x801A16B8u>::Invoke};
    for (auto invoke : cacheFunctions) {
        for (auto alias : aliases) {
            CopyAt(0);
            Cache(invoke, alias + 32768u, 1, {});
            Cache(invoke, alias + 32767u, 1, {backing[0]});
            CopyAt(1); // cache-line rounding covers the first byte.
            Cache(invoke, alias, 1, {backing[0] + 1});
            CopyAt(0);
            for (auto [address, length] : std::array<std::pair<std::uint32_t, std::uint32_t>, 6>{
                     {{alias, 0}, {0xfffffff0u, 32}, {0xffffffe0u, 32}, {alias, 0xffffffffu}, {alias + size - 1u, 2}, {alias + 1u, size}}})
                Cache(invoke, address, length, {});
            Cache(invoke, alias, 32768u, {backing[0]});
        }
    }
    CopyAt(0);
    CopyAt(32768);
    Evict(aliases[2] + 32767u, 2, {backing[0], backing[0] + 32768});
    CopyAt(0);
    mkw_switch_gx_forget_copy_destinations();
    Evict(base, size, {});
    for (auto [address, clear] : std::array<std::pair<std::uint32_t, std::uint32_t>, 9>{
             {{base, 0}, {base, 2}, {base, 0x10001u}, {base, 0xffffffffu}, {0, 1}, {aliases[1], 1}, {aliases[2], 1}, {0xffffffffu, 1}, {0x94000000u, 1}}})
        Refusal(address, clear, "GX_COPY_TEX_UNPROVEN_ARGS");
    for (auto address : {base - 1u, base + size - 32768u + 1u, 0x93ffffffu})
        Refusal(address, 1, "GX_COPY_TEX_DESTINATION_RANGE");
    for (unsigned member = 0; member < 8; ++member) {
        Configure();
        switch (member) {
        case 0:
            g_texCopyState.srcLeft = 1;
            break;
        case 1:
            g_texCopyState.srcTop = 1;
            break;
        case 2:
            g_texCopyState.srcWidth = 64;
            break;
        case 3:
            g_texCopyState.srcHeight = 64;
            break;
        case 4:
            g_texCopyState.dstWidth = 64;
            break;
        case 5:
            g_texCopyState.dstHeight = 64;
            break;
        case 6:
            g_texCopyState.dstFormat = 0x105;
            break;
        case 7:
            g_texCopyState.dstMipmap = 0x10000;
            break;
        }
        Refusal(base, 1, "GX_COPY_TEX_UNPROVEN_ARGS");
    }
    Configure();
    refusalPrefix = {Ensure, Snapshot};
    active = false;
    Refusal(base, 1, "GX_COPY_TEX_FRAME_INACTIVE");
    active = true;
    initialized = false;
    Refusal(base, 1, "GX_COPY_TEX_FRAME_INACTIVE");
    initialized = true;
    refusalPrefix = {Ensure, Snapshot, Drain};
    changeConfig = true;
    Refusal(base, 1, "GX_COPY_TEX_CONFIGURATION_CHANGED");
    changeConfig = false;
    refusalPrefix = {Ensure, Snapshot, Drain, Source, Copy};
    throwCopy = true;
    expectedDestination = backing[0];
    Refusal(base, 1, "GX_COPY_TEX_NATIVE_EXCEPTION");
    throwCopy = false;
    refusalPrefix.clear();
    CopyAt(0);
    reenterDestroy = true;
    Evict(base, 32768u, {backing[0]});
    keepMapping = true;
    Memory::Reset();
    keepMapping = false;
    Refusal(base, 1, "GX_COPY_TEX_DESTINATION_RANGE");
    Memory::Init(config);
#else
    Refusal(base, 1, "GX_COPY_TEX_REQUIRES_RENDERER");
    Refusal(0xffffffffu, 0xffffffffu, "GX_COPY_TEX_REQUIRES_RENDERER");
    auto cpu = Cpu(base, 32768);
    Prepare(cpu);
    mkw_switch_hle_dc_range(&cpu);
    CheckCpu();
    assert(events.empty());
    liveCpu = nullptr;
    mkw_switch_gx_invalidate_copy_destinations(base, size);
    assert(events.empty());
#endif
    CheckBytes();
    mkw_switch_gx_forget_copy_destinations();
    Memory::Reset();
    std::printf("PASS: GXCopyTex valid=%u refusals=%u rendered=%d\n", valid, refusals, MKW_LOCAL_RENDERED_FAST_TRACK);
}
