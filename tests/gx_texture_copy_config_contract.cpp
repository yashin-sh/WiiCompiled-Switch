#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "gx_internal.h"
#include "switch_gx_hle_traits.hpp"

#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <vector>

#if !MKW_LOCAL_RENDERED_FAST_TRACK
TexCopyState g_texCopyState;
#endif

namespace {
enum class Op : unsigned { Clamp,
                           Src,
                           Dst };
using Args = std::array<std::uint32_t, 4>;
struct Contract {
    std::uint32_t target;
    const char* stage;
    void (*invoke)(CpuContext*) noexcept;
};
constexpr std::array contracts{
    Contract{0x8016F618u, "RMCP01_GX_SET_COPY_CLAMP", KnownNativeCpuCall<0x8016F618u>::Invoke},
    Contract{0x8016F478u, "RMCP01_GX_SET_TEX_COPY_SRC", KnownNativeCpuCall<0x8016F478u>::Invoke},
    Contract{0x8016F4DCu, "RMCP01_GX_SET_TEX_COPY_DST", KnownNativeCpuCall<0x8016F4DCu>::Invoke},
};
constexpr std::uint32_t gd = 0x70002000u;
struct Region {
    std::uint32_t base;
    std::size_t size;
    void* mapping;
    std::size_t mappedSize;
    std::uint8_t* bytes;
};
std::vector<Region> regions;
const char* stage = nullptr;
unsigned stages = 0, nativeCalls = 0, lookups = 0, validCases = 0, abortCases = 0;
unsigned oldStages = 0, oldNative = 0;
bool observing = false;
Op operation = Op::Clamp;
CpuContext expectedCpu{};
CpuContext* liveCpu = nullptr;
TexCopyState oldShadow{};
int reportPipe = -1;
const char* expectedReason = nullptr;
unsigned reportCalls = 0;

const Contract& For(Op op) {
    return contracts[static_cast<unsigned>(op)];
}
CpuContext MakeCpu(const Args& args) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x5bu + 37u * i);
    for (std::size_t i = 0; i < args.size(); ++i)
        cpu.gpr[3u + i] = args[i];
    return cpu;
}
void Prepare(CpuContext& cpu, Op op) {
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    liveCpu = &cpu;
    operation = op;
    oldStages = stages;
    oldNative = nativeCalls;
    lookups = 0;
    oldShadow = g_texCopyState;
    observing = true;
}
void CheckCpu() {
    assert(liveCpu && std::memcmp(liveCpu, &expectedCpu, sizeof(expectedCpu)) == 0);
    assert(std::strcmp(stage, For(operation).stage) == 0 && stages == oldStages + 1u);
}
void Observe(Op op) {
    if (reportPipe >= 0)
        _exit(91);
    assert(op == operation && nativeCalls == oldNative && lookups == 0);
    CheckCpu();
    assert(std::memcmp(&g_texCopyState, &oldShadow, sizeof(oldShadow)) == 0);
    ++nativeCalls;
}
void Invoke(Op op, const Args& args) {
    auto cpu = MakeCpu(args);
    Prepare(cpu, op);
    For(op).invoke(&cpu);
    CheckCpu();
    assert(nativeCalls == oldNative + 1u);
    auto expected = oldShadow;
    if (op == Op::Src) {
        expected.srcLeft = static_cast<std::uint16_t>(args[0]);
        expected.srcTop = static_cast<std::uint16_t>(args[1]);
        expected.srcWidth = static_cast<std::uint16_t>(args[2]);
        expected.srcHeight = static_cast<std::uint16_t>(args[3]);
    } else if (op == Op::Dst) {
        expected.dstWidth = static_cast<std::uint16_t>(args[0]);
        expected.dstHeight = static_cast<std::uint16_t>(args[1]);
        expected.dstFormat = args[2];
        expected.dstMipmap = args[3];
    }
    assert(std::memcmp(&g_texCopyState, &expected, sizeof(expected)) == 0);
    if (op != Op::Clamp)
        assert(lookups == 0);
    observing = false;
    liveCpu = nullptr;
    ++validCases;
}
using Snapshot = std::vector<std::vector<std::uint8_t>>;
Snapshot Capture() {
    Snapshot out;
    for (const auto& r : regions)
        out.emplace_back(r.bytes, r.bytes + r.size);
    return out;
}
void CheckMemory(const Snapshot& before) {
    for (std::size_t i = 0; i < regions.size(); ++i)
        assert(std::memcmp(before[i].data(), regions[i].bytes, regions[i].size) == 0);
}
void ExpectAbort(Op op, const Args& args, const char* reason) {
    const auto before = Capture();
    const auto shadow = g_texCopyState;
    auto initial = MakeCpu(args);
    auto* shared = static_cast<CpuContext*>(mmap(nullptr, sizeof(CpuContext), PROT_READ | PROT_WRITE,
                                                 MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    assert(shared != MAP_FAILED);
    new (shared) CpuContext;
    std::memcpy(shared, &initial, sizeof(initial));
    int descriptors[2];
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(descriptors[0]);
        reportPipe = descriptors[1];
        expectedReason = reason;
        Prepare(*shared, op);
        For(op).invoke(shared);
        _exit(90);
    }
    close(descriptors[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    std::array<char, 2> marker{};
    assert(read(descriptors[0], marker.data(), marker.size()) == 1 && marker[0] == 'R');
    close(descriptors[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(std::memcmp(shared, &initial, sizeof(initial)) == 0);
    assert(std::memcmp(&g_texCopyState, &shadow, sizeof(shadow)) == 0);
    CheckMemory(before);
    assert(munmap(shared, sizeof(CpuContext)) == 0);
    ++abortCases;
}
void CheckNull() {
    const auto before = Capture();
    const auto shadow = g_texCopyState;
    const auto oldStage = stage;
    const auto oldCount = stages;
    const auto oldCalls = nativeCalls;
    for (const auto& c : contracts)
        c.invoke(nullptr);
    assert(stage == oldStage && stages == oldCount && nativeCalls == oldCalls);
    assert(std::memcmp(&g_texCopyState, &shadow, sizeof(shadow)) == 0);
    CheckMemory(before);
}
void Init(std::uint32_t base = gd, std::size_t size = 0x260u, bool pointer = true) {
    Memory::Config config;
    if (pointer)
        config.regions.push_back({"gx-pointer", kGXDataPtrAddr, 4u});
    config.regions.push_back({"gx-data", base, size});
    Memory::Init(config);
    if (pointer)
        Memory::Write32(kGXDataPtrAddr, base);
}
[[maybe_unused]] void CheckClamp(std::uint32_t base, std::size_t size, std::uint32_t clamp) {
    Init(base, size);
    const auto disp = base + 0x23Cu;
    const auto tex = base + 0x24Cu;
    if (Memory::Contains(disp, 4u))
        Memory::Write32(disp, 0x1234567bu);
    if (Memory::Contains(tex, 4u))
        Memory::Write32(tex, 0xabcdef03u);
    const auto before = Capture();
    Invoke(Op::Clamp, {clamp, 0xdeadbeefu, 0xaabbccddu, 0xffffffffu});
    // Independent byte fixture: only the final byte's two low bits change.
    auto expected = before;
    if (base && Memory::Contains(disp, 4u)) {
        for (std::size_t i = 0; i < regions.size(); ++i)
            if (disp >= regions[i].base && std::uint64_t(disp) + 4u <= std::uint64_t(regions[i].base) + regions[i].size)
                expected[i][disp - regions[i].base + 3u] = static_cast<std::uint8_t>(0x78u | clamp);
        if (Memory::Contains(tex, 4u))
            for (std::size_t i = 0; i < regions.size(); ++i)
                if (tex >= regions[i].base && std::uint64_t(tex) + 4u <= std::uint64_t(regions[i].base) + regions[i].size)
                    expected[i][tex - regions[i].base + 3u] = static_cast<std::uint8_t>(clamp);
    }
    CheckMemory(expected);
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
        regions.push_back({r.base, r.size, mapping, 3u * page, bytes});
    }
}
std::uint8_t* HostPointer(std::uint32_t addr) {
    for (const auto& r : regions)
        if (addr >= r.base && std::uint64_t(addr) < std::uint64_t(r.base) + r.size)
            return r.bytes + (addr - r.base);
    return nullptr;
}
} // namespace GuestFlat

extern "C" std::uint8_t* __real__ZN6Memory10GetPointerEjm(std::uint32_t, std::size_t);
extern "C" std::uint8_t* __wrap__ZN6Memory10GetPointerEjm(std::uint32_t addr, std::size_t length) {
    if (observing) {
        if (reportPipe >= 0)
            _exit(92);
        assert(operation == Op::Clamp && nativeCalls == oldNative + 1u && length == 4u);
        CheckCpu();
        ++lookups;
    }
    return __real__ZN6Memory10GetPointerEjm(addr, length);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stages;
}
extern "C" void GXSetCopyClamp(GXFBClamp value) {
    Observe(Op::Clamp);
    assert(static_cast<std::uint32_t>(value) == expectedCpu.gpr[3]);
}
extern "C" void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height) {
    Observe(Op::Src);
    const std::array<u16, 4> values{left, top, width, height};
    for (unsigned i = 0; i < 4u; ++i)
        assert(values[i] == (expectedCpu.gpr[3u + i] & 0xffffu));
}
extern "C" void GXSetTexCopyDst(u16 width, u16 height, GXTexFmt format, GXBool mipmap) {
    static_assert(std::is_same_v<GXBool, bool>);
    Observe(Op::Dst);
    assert(width == (expectedCpu.gpr[3] & 0xffffu) && height == (expectedCpu.gpr[4] & 0xffffu));
    assert(format == GX_TF_RGB5A3 && mipmap == (expectedCpu.gpr[6] != 0u));
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    if (reportPipe < 0 || reportCalls++ != 0u || cpu != liveCpu || target != For(operation).target || std::strcmp(reason, expectedReason) != 0 ||
        nativeCalls != oldNative || lookups != 0)
        _exit(93);
    CheckCpu();
    assert(std::memcmp(&g_texCopyState, &oldShadow, sizeof(oldShadow)) == 0);
    assert(write(reportPipe, "R", 1) == 1);
}
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort() {
    if (reportPipe >= 0) {
        if (reportCalls != 1u || nativeCalls != oldNative || lookups != 0 ||
            std::memcmp(&g_texCopyState, &oldShadow, sizeof(oldShadow)) != 0 ||
            std::memcmp(liveCpu, &expectedCpu, sizeof(expectedCpu)) != 0)
            _exit(94);
    }
    __real_abort();
}

int main() {
    g_texCopyState = {0x12u, 0x34u, 0x56u, 0x78u, 0x9au, 0xbcu, 5u, 0xdeadbeefu};
    CheckNull();
#if MKW_LOCAL_RENDERED_FAST_TRACK
    Invoke(Op::Clamp, {3u}); // Native still runs when guest memory is absent.
    for (std::uint32_t c = 0; c < 4u; ++c) {
        CheckClamp(gd, 0x260u, c);
        CheckClamp(gd + 1u, 0x260u, c); // Unaligned words remain supported.
        CheckClamp(0u, 0x260u, c);      // A null GXData pointer skips the guest mirror.
        CheckClamp(gd, 0x240u, c);      // First word commits, second read fails.
        CheckClamp(gd, 0x23Fu, c);      // A short first word cannot be touched.
        Init(gd, 0x260u, false);
        Invoke(Op::Clamp, {c});
    }
    Init();
    Memory::Write32(kGXDataPtrAddr, 0xfffffdc4u); // First addition wraps to zero.
    Invoke(Op::Clamp, {3u});                      // Unmapped wrapped address is swallowed, not cast.
    Memory::Write32(kGXDataPtrAddr, 0x60000000u);
    Invoke(Op::Clamp, {3u});
    Init(0u, 32u);
    Memory::Write32(kGXDataPtrAddr, 0xfffffdc4u);
    Memory::Write32(0u, 0x13579bdfu);
    Memory::Write32(0x10u, 0x2468ace2u);
    const auto wrappedBefore = Capture();
    Invoke(Op::Clamp, {1u});
    auto wrappedExpected = wrappedBefore;
    wrappedExpected.back()[3] = 0xddu;
    wrappedExpected.back()[0x13] = 0xe1u;
    CheckMemory(wrappedExpected);
    Init();
    for (auto c : {4u, 255u, 256u, 0x10003u, 0x80000003u, 0xffffffffu})
        ExpectAbort(Op::Clamp, {c}, "GX_COPY_CLAMP_UNPROVEN_ARGS");
    for (auto f : {0u, 4u, 6u, 0x105u, 0x80000005u, 0xffffffffu})
        ExpectAbort(Op::Dst, {128u, 128u, f, 0u}, "GX_TEX_COPY_DST_UNPROVEN_FORMAT");
    const auto before = Capture();
    Invoke(Op::Src, {0u, 0u, 128u, 128u});
    Invoke(Op::Dst, {128u, 128u, 5u, 0u});
    for (unsigned component = 0; component < 4u; ++component)
        for (std::uint32_t value = 0; value < 65536u; ++value) {
            Args a{0x12340012u, 0xabcd0034u, 0xfedc0056u, 0xffff0078u};
            a[component] = 0xabcd0000u | value;
            Invoke(Op::Src, a);
        }
    for (unsigned component = 0; component < 2u; ++component)
        for (std::uint32_t value = 0; value < 65536u; ++value) {
            Args a{0x1234009au, 0xffff00bcu, 5u, 0x10000u};
            a[component] = 0xabcd0000u | value;
            Invoke(Op::Dst, a);
        }
    for (auto m : {0u, 1u, 255u, 256u, 0x10000u, 0x80000000u, 0xffffffffu})
        Invoke(Op::Dst, {128u, 128u, 5u, m});
    CheckMemory(before);
#else
    Init();
    for (const Args a : {Args{0u, 0u, 128u, 128u}, {3u, 0u, 0u, 0u}, {128u, 128u, 5u, 0u}, {0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu}})
        for (auto op : {Op::Clamp, Op::Src, Op::Dst})
            ExpectAbort(op, a, "GX_TEXTURE_COPY_REQUIRES_RENDERER");
#endif
    CheckNull();
    Memory::Reset();
    std::printf("PASS: texture-copy config valid=%u diagnosed-aborts=%u native=%u rendered=%d\n", validCases, abortCases, nativeCalls, MKW_LOCAL_RENDERED_FAST_TRACK);
}
