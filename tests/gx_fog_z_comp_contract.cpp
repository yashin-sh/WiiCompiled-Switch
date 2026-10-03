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
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <type_traits>

namespace {
enum class Operation : std::size_t { Fog,
                                     ZComp };
using Args = std::array<std::uint32_t, 5>;
struct Contract {
    std::uint32_t target;
    const char* stage;
    const char* reason;
    const char* memoryReason;
    void (*invoke)(CpuContext*) noexcept;
};
constexpr std::array contracts{
    Contract{0x801722ccu, "RMCP01_GX_SET_FOG", "GX_SET_FOG_UNPROVEN_ARGS", "GX_SET_FOG_UNREADABLE_COLOR", KnownNativeCpuCall<0x801722ccu>::Invoke},
    Contract{0x80172858u, "RMCP01_GX_SET_Z_COMP_LOC", nullptr, nullptr, KnownNativeCpuCall<0x80172858u>::Invoke},
};
constexpr std::array<std::uint64_t, 4> observedBits{
    0x0000000000000000ull, 0x3ff0000000000000ull,
    0x3fb99999a0000000ull, 0x3ff0000000000000ull};
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
Operation expectedOperation = Operation::Fog;
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
    for (std::size_t i = 0; i < observedBits.size(); ++i)
        std::memcpy(&cpu.fpr[1u + i].d, &observedBits[i], sizeof(observedBits[i]));
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
    assert(lookupCalls == previousLookupCalls + (op == Operation::ZComp ? 0u : 1u));
    ++nativeCalls;
}
void InvokeAndCheck(Operation op, const Args& args) {
    auto cpu = MakeCpu(args);
    const auto memory = CaptureMemory();
    Prepare(cpu, op);
    For(op).invoke(&cpu);
    CheckCpuAndStage();
    assert(lookupCalls == previousLookupCalls + (op == Operation::ZComp ? 0u : 1u));
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == previousNativeCalls + 1u);
#else
    assert(nativeCalls == previousNativeCalls);
#endif
    CheckMemory(memory);
    liveCpu = nullptr;
    ++validCases;
}
void ExpectAbort(const Args& args, bool memoryFailure = false, int changedFpr = -1, std::uint64_t bits = 0) {
    const auto op = Operation::Fog;
    const auto memory = CaptureMemory();
    void* storage = mmap(nullptr, sizeof(CpuContext), PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert(storage != MAP_FAILED);
    auto* sharedCpu = new (storage) CpuContext;
    auto initial = MakeCpu(args);
    if (changedFpr >= 0)
        std::memcpy(&initial.fpr[1 + changedFpr].d, &bits, sizeof(bits));
    std::memcpy(sharedCpu, &initial, sizeof(initial));
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        // Pipe-based system core collectors can ignore RLIMIT_CORE=0. Keep
        // the real SIGABRT contract without invoking the collector hundreds
        // of times. Only this child becomes nondumpable; parent LSan stays on.
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
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
    InvokeAndCheck(Operation::Fog, {0u, pointer, 0xdeadbeefu, 0x12345678u, 0xffffffffu});
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
    assert(expectedOperation != Operation::ZComp);
    assert(pointer == expectedCpu.gpr[4] && length == 4u);
    ++lookupCalls;
    return __real__ZN6Memory10GetPointerEjm(pointer, length);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stageCalls;
}
extern "C" void GXSetFog(GXFogType type, float start, float end, float near, float far, GXColor color) {
    Observe(Operation::Fog);
    assert(type == GX_FOG_NONE);
    // Independent expected native representation; catches .f instead of .d,
    // swapped FPRs and constant/incorrect rounding of the captured near plane.
    const std::array<float, 4> values{start, end, near, far};
    constexpr std::array<std::uint32_t, 4> expected{0u, 0x3f800000u, 0x3dcccccdu, 0x3f800000u};
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::uint32_t bits;
        std::memcpy(&bits, &values[i], sizeof(bits));
        assert(bits == expected[i]);
    }
    assert(color.r == expectedColor[0] && color.g == expectedColor[1]);
    assert(color.b == expectedColor[2] && color.a == expectedColor[3]);
}
extern "C" void GXSetZCompLoc(GXBool beforeTexture) {
    static_assert(std::is_same_v<GXBool, bool>);
    Observe(Operation::ZComp);
    assert(beforeTexture == (expectedCpu.gpr[3] != 0u));
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    if (reportPipe < 0 || reportCalls++)
        _exit(92);
    if (cpu != liveCpu || target != 0x801722ccu || std::strcmp(reason, expectedReason) != 0 ||
        lookupCalls != previousLookupCalls + expectedLookups || nativeCalls != previousNativeCalls)
        _exit(94);
    CheckCpuAndStage();
    assert(write(reportPipe, "R", 1) == 1);
}
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort() {
    if (reportPipe >= 0) {
        if (reportCalls != 1u || nativeCalls != previousNativeCalls ||
            lookupCalls != previousLookupCalls + expectedLookups)
            _exit(95);
        CheckCpuAndStage();
    }
    __real_abort();
}

int main() {
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    CheckNull();
    ExpectAbort({1u, address}); // Invalid tuple before memory initialization.
    ExpectAbort({0u, address}, true);
    Memory::Config config;
    config.regions.push_back({"physical-zero", 0u, 64u});
    config.regions.push_back({"fog-test", address, 64u});
    config.regions.push_back({"wrap-test", 0xfffffff0u, 16u});
    Memory::Init(config);
    for (auto pointer : {0u, 1u, 60u, address, address + 1u, address + 60u, 0xfffffff0u, 0xfffffff1u, 0xfffffffcu})
        for (const std::array<std::uint8_t, 4> color : {std::array<std::uint8_t, 4>{0, 0, 0, 0}, {255, 255, 255, 255}, {0x12, 0x34, 0x56, 0x78}, {0x80, 0xff, 0x01, 0x7f}})
            CheckColor(pointer, color);
    for (std::size_t component = 0; component < 4u; ++component)
        for (std::uint32_t value = 0; value < 256u; ++value) {
            std::array<std::uint8_t, 4> color{0x12, 0x34, 0x56, 0x78};
            color[component] = static_cast<std::uint8_t>(value);
            CheckColor(address + 1u, color);
        }
    // Every single-bit departure must refuse before memory or native work,
    // including f64 deviations which would disappear after narrowing to f32.
    for (int fpr = 0; fpr < 4; ++fpr)
        for (unsigned bit = 0; bit < 64u; ++bit)
            ExpectAbort({0u, 0x60000000u}, false, fpr, observedBits[fpr] ^ (std::uint64_t{1} << bit));
    for (int fpr = 0; fpr < 4; ++fpr)
        for (const std::uint64_t bits : {0x7ff0000000000000ull, 0xfff0000000000000ull,
                                         0x7ff8000000000000ull, 0x7ff0000000000001ull,
                                         0x7fefffffffffffffull, 0xbff0000000000000ull})
            ExpectAbort({0u, address}, false, fpr, bits);
    for (auto type : {1u, 2u, 4u, 5u, 7u, 10u, 15u, 256u, 0x80000000u, 0xffffffffu})
        for (auto pointer : {address, 0x60000000u})
            ExpectAbort({type, pointer});
    for (auto pointer : {61u, 64u, address - 1u, address + 61u, address + 64u, 0x60000000u, 0xfffffffdu, 0xfffffffeu, 0xffffffffu})
        ExpectAbort({0u, pointer}, true);
    for (std::uint32_t value = 0; value < 65536u; ++value)
        InvokeAndCheck(Operation::ZComp, {value, 0x60000000u});
    for (auto value : {0x10000u, 0x100ffu, 0x80000000u, 0xffffff00u, 0xffffffffu})
        InvokeAndCheck(Operation::ZComp, {value, 0x60000000u});
    CheckNull();
    Memory::Reset();
    ExpectAbort({0u, 0u}, true);
    assert(validCases == 66601u && abortCases == 312u);
    std::printf("PASS: Fog/ZComp valid=%u diagnosed-aborts=%u native=%u rendered=%d\n", validCases, abortCases, nativeCalls, MKW_LOCAL_RENDERED_FAST_TRACK);
}
