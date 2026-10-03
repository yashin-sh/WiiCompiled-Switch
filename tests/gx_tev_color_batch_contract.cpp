#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
enum class Operation : std::size_t { KColor,
                                     Color,
                                     SwapTable };
using Args = std::array<std::uint32_t, 5>;
struct Contract {
    std::uint32_t target;
    const char* stage;
    const char* reason;
    const char* memoryReason;
    void (*invoke)(CpuContext*) noexcept;
};
constexpr std::array contracts{
    Contract{0x80171ed4u, "RMCP01_GX_SET_TEV_K_COLOR", "GX_SET_TEV_K_COLOR_UNPROVEN_ARGS", "GX_SET_TEV_K_COLOR_UNREADABLE_COLOR", KnownNativeCpuCall<0x80171ed4u>::Invoke},
    Contract{0x80171e10u, "RMCP01_GX_SET_TEV_COLOR", "GX_SET_TEV_COLOR_UNPROVEN_ARGS", "GX_SET_TEV_COLOR_UNREADABLE_COLOR", KnownNativeCpuCall<0x80171e10u>::Invoke},
    Contract{0x8017200cu, "RMCP01_GX_SET_TEV_SWAP_MODE_TABLE", "GX_SET_TEV_SWAP_MODE_TABLE_UNPROVEN_ARGS", nullptr, KnownNativeCpuCall<0x8017200cu>::Invoke},
};
constexpr std::uint32_t address = 0x70002000u;
const char* stage = nullptr;
std::uint32_t stageCalls = 0;
std::uint32_t lookupCalls = 0;
std::uint32_t nativeCalls = 0;
std::uint32_t previousStageCalls = 0;
std::uint32_t previousLookupCalls = 0;
std::uint32_t previousNativeCalls = 0;
std::uint32_t validCases = 0;
std::uint32_t abortCases = 0;
Operation expectedOperation = Operation::KColor;
CpuContext expectedCpu{};
CpuContext* liveCpu = nullptr;
std::array<std::uint8_t, 4> expectedColor{};
int reportPipe = -1;
const char* expectedReason = nullptr;
std::uint32_t expectedLookups = 0;
std::uint32_t reportCalls = 0;
struct HostRegion {
    std::uint32_t base;
    std::size_t size;
    void* allocation;
    std::size_t allocationSize;
    std::uint8_t* bytes;
};
std::vector<HostRegion> regions;
using MemorySnapshot = std::vector<std::vector<std::uint8_t>>;

const Contract& For(Operation op) {
    return contracts[static_cast<std::size_t>(op)];
}
MemorySnapshot CaptureMemory() {
    MemorySnapshot result;
    for (const auto& r : regions)
        result.emplace_back(r.bytes, r.bytes + r.size);
    return result;
}
void CheckMemory(const MemorySnapshot& before) {
    assert(before.size() == regions.size());
    for (std::size_t i = 0; i < regions.size(); ++i)
        assert(std::memcmp(before[i].data(), regions[i].bytes, regions[i].size) == 0);
}
CpuContext MakeCpu(const Args& args) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x43u + i * 37u);
    for (std::size_t i = 0; i < args.size(); ++i)
        cpu.gpr[3u + i] = args[i];
    return cpu;
}
void Prepare(CpuContext& cpu, Operation op) {
    liveCpu = &cpu;
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    expectedOperation = op;
    previousStageCalls = stageCalls;
    previousLookupCalls = lookupCalls;
    previousNativeCalls = nativeCalls;
    stage = "PREVIOUS_STAGE";
}
void CheckCpuAndStage() {
    assert(liveCpu && std::memcmp(liveCpu, &expectedCpu, sizeof(expectedCpu)) == 0);
    assert(stage && std::strcmp(stage, For(expectedOperation).stage) == 0);
    assert(stageCalls == previousStageCalls + 1u);
}
void Observe(Operation op) {
    if (reportPipe >= 0)
        _exit(91); // Never mistake a native assertion for the required abort.
    assert(op == expectedOperation && nativeCalls == previousNativeCalls);
    CheckCpuAndStage();
    assert(lookupCalls == previousLookupCalls + (op == Operation::SwapTable ? 0u : 1u));
    ++nativeCalls;
}
void InvokeAndCheck(Operation op, const Args& args) {
    auto cpu = MakeCpu(args);
    const auto memory = CaptureMemory();
    Prepare(cpu, op);
    For(op).invoke(&cpu);
    CheckCpuAndStage();
    assert(lookupCalls == previousLookupCalls + (op == Operation::SwapTable ? 0u : 1u));
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == previousNativeCalls + 1u);
#else
    assert(nativeCalls == previousNativeCalls);
#endif
    CheckMemory(memory);
    liveCpu = nullptr;
    ++validCases;
}
void ExpectAbort(Operation op, const Args& args, bool memoryFailure = false) {
    const auto memory = CaptureMemory();
    void* storage = mmap(nullptr, sizeof(CpuContext), PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert(storage != MAP_FAILED);
    auto* sharedCpu = new (storage) CpuContext;
    const auto initial = MakeCpu(args);
    std::memcpy(sharedCpu, &initial, sizeof(initial));
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        reportPipe = descriptors[1];
        expectedReason = memoryFailure ? For(op).memoryReason : For(op).reason;
        expectedLookups = memoryFailure ? 1u : 0u;
        Prepare(*sharedCpu, op);
        For(op).invoke(sharedCpu);
        _exit(90);
    }
    close(descriptors[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    std::array<char, 2> marker{};
    assert(read(descriptors[0], marker.data(), marker.size()) == 1 && marker[0] == 'R');
    close(descriptors[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    // CPU and guest backing are shared: mutations after reporting are visible.
    assert(std::memcmp(sharedCpu, &initial, sizeof(initial)) == 0);
    CheckMemory(memory);
    sharedCpu->~CpuContext();
    assert(munmap(storage, sizeof(CpuContext)) == 0);
    ++abortCases;
}
void CheckNull() {
    const auto oldStage = stage;
    const auto oldStages = stageCalls;
    const auto oldLookups = lookupCalls;
    const auto oldNative = nativeCalls;
    for (const auto& c : contracts)
        c.invoke(nullptr);
    assert(stage == oldStage && stageCalls == oldStages);
    assert(lookupCalls == oldLookups && nativeCalls == oldNative);
}
void CheckColor(std::uint32_t pointer, const std::array<std::uint8_t, 4>& color) {
    // Independent raw-byte fixture; no Read32 conversion or host word casting.
    std::memcpy(GuestFlat::HostPointer(pointer), color.data(), color.size());
    expectedColor = color;
    for (std::uint32_t id = 0; id < 4u; ++id)
        for (auto op : {Operation::KColor, Operation::Color})
            InvokeAndCheck(op, {id, pointer, 0xdeadbeefu, 0x12345678u, 0xffffffffu});
}
} // namespace

// Allocation-only Horizon seam. Actual Memory::GetPointer checks the range.
// Each region ends at a protected page to expose even one-byte overreads;
// MAP_SHARED makes child guest-memory mutations visible after refusal.
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Shutdown() noexcept {
    for (const auto& r : regions)
        assert(munmap(r.allocation, r.allocationSize) == 0);
    regions.clear();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    Shutdown();
    const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    for (const auto& r : requests) {
        assert(r.size <= page);
        void* allocation = mmap(nullptr, page * 3u, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        assert(allocation != MAP_FAILED);
        auto* middle = static_cast<std::uint8_t*>(allocation) + page;
        assert(mprotect(middle, page, PROT_READ | PROT_WRITE) == 0);
        auto* bytes = middle + page - r.size;
        std::memset(bytes, 0x9b, r.size);
        regions.push_back({r.base, r.size, allocation, page * 3u, bytes});
    }
}
std::uint8_t* HostPointer(std::uint32_t pointer) {
    for (const auto& r : regions)
        if (pointer >= r.base && std::uint64_t(pointer) < std::uint64_t(r.base) + r.size)
            return r.bytes + (pointer - r.base);
    return nullptr;
}
} // namespace GuestFlat

// GNU linker instrumentation for the Linux 64-bit host contract only.
// Count bridge lookups while delegating to the actual Switch Memory method.
static_assert(sizeof(std::size_t) == 8u);
extern "C" std::uint8_t* __real__ZN6Memory10GetPointerEjm(std::uint32_t, std::size_t);
extern "C" std::uint8_t* __wrap__ZN6Memory10GetPointerEjm(std::uint32_t pointer, std::size_t length) {
    if (reportPipe >= 0 && reportCalls != 0u)
        _exit(93); // A refusal cannot perform a lookup after its report either.
    CheckCpuAndStage();
    assert(expectedOperation != Operation::SwapTable);
    assert(pointer == expectedCpu.gpr[4] && length == 4u);
    ++lookupCalls;
    return __real__ZN6Memory10GetPointerEjm(pointer, length);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stageCalls;
}
extern "C" void GXSetTevKColor(GXTevKColorID id, GXColor color) {
    Observe(Operation::KColor);
    assert(static_cast<std::uint32_t>(id) == expectedCpu.gpr[3]);
    assert(color.r == expectedColor[0] && color.g == expectedColor[1]);
    assert(color.b == expectedColor[2] && color.a == expectedColor[3]);
}
extern "C" void GXSetTevColor(GXTevRegID id, GXColor color) {
    Observe(Operation::Color);
    assert(static_cast<std::uint32_t>(id) == expectedCpu.gpr[3]);
    assert(color.r == expectedColor[0] && color.g == expectedColor[1]);
    assert(color.b == expectedColor[2] && color.a == expectedColor[3]);
}
extern "C" void GXSetTevSwapModeTable(GXTevSwapSel id, GXTevColorChan r, GXTevColorChan g, GXTevColorChan b, GXTevColorChan a) {
    Observe(Operation::SwapTable);
    const std::array<std::uint32_t, 5> values{static_cast<std::uint32_t>(id), static_cast<std::uint32_t>(r), static_cast<std::uint32_t>(g), static_cast<std::uint32_t>(b), static_cast<std::uint32_t>(a)};
    for (std::size_t i = 0; i < values.size(); ++i)
        assert(values[i] == expectedCpu.gpr[3u + i]);
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    if (reportPipe < 0 || reportCalls++)
        _exit(92);
    assert(cpu == liveCpu && target == For(expectedOperation).target);
    assert(std::strcmp(reason, expectedReason) == 0);
    assert(lookupCalls == previousLookupCalls + expectedLookups);
    assert(nativeCalls == previousNativeCalls);
    CheckCpuAndStage();
    assert(write(reportPipe, "R", 1) == 1);
}

int main() {
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    CheckNull();
    for (auto op : {Operation::KColor, Operation::Color}) {
        ExpectAbort(op, {4u, address}); // Invalid ID before memory initialization.
        ExpectAbort(op, {0u, address}, true);
    }
    Memory::Config config;
    config.regions.push_back({"physical-zero", 0u, 64u});
    config.regions.push_back({"color-test", address, 64u});
    config.regions.push_back({"wrap-test", 0xfffffff0u, 16u});
    Memory::Init(config);
    for (auto pointer : {0u, 1u, 60u, address, address + 1u, address + 60u, 0xfffffff0u, 0xfffffff1u, 0xfffffffcu})
        for (const std::array<std::uint8_t, 4> color : {std::array<std::uint8_t, 4>{0, 0, 0, 0}, {255, 255, 255, 255}, {0x12, 0x34, 0x56, 0x78}, {0x80, 0xff, 0x01, 0x7f}})
            CheckColor(pointer, color);
    // Each channel's full byte domain, asymmetric neighbors and every ID.
    for (std::size_t component = 0; component < 4u; ++component)
        for (std::uint32_t value = 0; value < 256u; ++value) {
            std::array<std::uint8_t, 4> color{0x12, 0x34, 0x56, 0x78};
            color[component] = static_cast<std::uint8_t>(value);
            CheckColor(address + 1u, color);
        }
    for (std::uint32_t id = 0; id < 4u; ++id)
        for (std::uint32_t r = 0; r < 4u; ++r)
            for (std::uint32_t g = 0; g < 4u; ++g)
                for (std::uint32_t b = 0; b < 4u; ++b)
                    for (std::uint32_t a = 0; a < 4u; ++a)
                        InvokeAndCheck(Operation::SwapTable, {id, r, g, b, a});
    for (auto op : {Operation::KColor, Operation::Color}) {
        for (auto id : {4u, 16u, 256u, 0xffffu, 0x80000000u, 0xffffffffu})
            for (auto pointer : {address, 0x60000000u})
                ExpectAbort(op, {id, pointer});
        for (std::uint32_t id = 0; id < 4u; ++id)
            for (auto pointer : {61u, 64u, address - 1u, address + 61u, address + 64u, 0x60000000u, 0xfffffffdu, 0xfffffffeu, 0xffffffffu})
                ExpectAbort(op, {id, pointer}, true);
    }
    for (std::size_t position = 0; position < 5u; ++position)
        for (auto bad : {4u, 16u, 256u, 0xffffu, 0x80000000u, 0xffffffffu}) {
            Args args{3u, 0u, 1u, 2u, 3u};
            args[position] = bad;
            ExpectAbort(Operation::SwapTable, args);
        }
    CheckNull();
    Memory::Reset();
    for (auto op : {Operation::KColor, Operation::Color})
        ExpectAbort(op, {0u, 0u}, true);
    std::printf("PASS: TEV color batch valid=%u diagnosed-aborts=%u native=%u rendered=%d\n", validCases, abortCases, nativeCalls, MKW_LOCAL_RENDERED_FAST_TRACK);
}
