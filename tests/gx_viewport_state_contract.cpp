#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "gx_internal.h"
#include "switch_gx_hle_traits.hpp"
#include <array>
#include <bit>
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

extern "C" {
int g_gxFrameCount = 0;
}
#if !MKW_LOCAL_RENDERED_FAST_TRACK
extern "C" {
float g_viewportState[6] = {0, 0, 1, 1, 0, 1};
}
#endif
#include "pinned-single-bits.inc"
namespace {
constexpr std::uint32_t out = 0x70001000u, node = 0x70002000u, screen = 0x70003000u, gd = 0x70004000u;
constexpr std::uint32_t onList = 0x809C1830u, offList = 0x809C183Cu;
struct Region {
    std::uint32_t base;
    std::size_t size, mappedSize;
    std::uint8_t* mapping;
    std::uint8_t* bytes;
};
std::vector<Region> regions;
using Snapshot = std::vector<std::vector<std::uint8_t>>;
Snapshot expected, beforeNative;
CpuContext cpu{}, saved{};
const char* stage = nullptr;
unsigned stages = 0, nativeCalls = 0, setters = 0, getters = 0, depth = 0;
bool throwing = false;
int proofFd = -1;
unsigned reports = 0, refusalNativeCount = 0, offsets = 0;
const char* reason = nullptr;
std::uint32_t refusalTarget = 0;
Snapshot Capture() {
    Snapshot result;
    for (const auto& r : regions)
        result.emplace_back(r.bytes, r.bytes + r.size);
    return result;
}
void Check(const Snapshot& bytes) {
    assert(bytes.size() == regions.size());
    for (std::size_t i = 0; i < regions.size(); ++i)
        assert(std::memcmp(regions[i].bytes, bytes[i].data(), regions[i].size) == 0);
}
void ExpectedBytes(std::uint32_t address, std::uint32_t value, unsigned count) {
    for (std::size_t i = 0; i < regions.size(); ++i) {
        const auto& r = regions[i];
        if (address >= r.base && std::uint64_t(address) + count <= std::uint64_t(r.base) + r.size) {
            for (unsigned j = 0; j < count; ++j)
                expected[i][address - r.base + j] = static_cast<std::uint8_t>(value >> ((count - 1u - j) * 8u));
            return;
        }
    }
}
void Save() {
    std::memcpy(&saved, &cpu, sizeof(cpu));
}
void CheckCpu() {
    assert(std::memcmp(&cpu, &saved, sizeof(cpu)) == 0);
}
void Init(std::size_t outputSize = 24, bool lists = true, std::size_t screenSize = 64, std::uint32_t outputBase = out) {
    Memory::Config config;
    if (outputSize)
        config.regions.push_back({"output", outputBase, outputSize});
    config.regions.push_back({"node", node, 0x1000});
    if (screenSize)
        config.regions.push_back({"screen", screen, screenSize});
    config.regions.push_back({"gx-pointer", kGXDataPtrAddr, 4});
    config.regions.push_back({"gx-data", gd, 0x600});
    if (lists) {
        config.regions.push_back({"onscreen", onList, 12});
        config.regions.push_back({"offscreen", offList, 12});
    }
    Memory::Init(config);
    if (lists) {
        Memory::Write32(onList, 0);
        Memory::Write16(onList + 10, 0);
        Memory::Write32(offList, 0);
        Memory::Write16(offList + 10, 0);
    }
    Memory::Write32(kGXDataPtrAddr, gd);
    ++g_gxFrameCount;
}
void Set(const std::array<double, 6>& args) {
    for (unsigned i = 0; i < 6; ++i)
        cpu.fpr[i + 1].d = args[i];
    Save();
    const auto before = Capture();
    const auto count = nativeCalls;
    KnownNativeCpuCall<0x801733B4u>::Invoke(&cpu);
    CheckCpu();
    Check(before);
    assert(std::strcmp(stage, "RMCP01_GX_SET_VIEWPORT") == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == count + 1);
    for (unsigned i = 0; i < 6; ++i)
        assert(std::bit_cast<std::uint32_t>(g_viewportState[i]) == std::bit_cast<std::uint32_t>(static_cast<float>(args[i])));
#else
    assert(nativeCalls == count);
#endif
    ++setters;
}
[[maybe_unused]] void Get(std::uint32_t address, Snapshot wanted) {
    cpu.gpr[3] = address;
    Save();
    expected = std::move(wanted);
    if (address)
        for (unsigned i = 0; i < 6; ++i)
            if (const auto a = address + i * 4u; a)
                ExpectedBytes(a, std::bit_cast<std::uint32_t>(g_viewportState[i]), 4);
    const auto count = nativeCalls;
    KnownNativeCpuCall<0x801733E0u>::Invoke(&cpu);
    CheckCpu();
    Check(expected);
    assert(std::strcmp(stage, "RMCP01_GX_GET_VIEWPORT") == 0 && nativeCalls == count);
    ++getters;
}
void Refusal(std::uint32_t target, const char* value, bool nativeError = false) {
    Save();
    expected = Capture();
    beforeNative = expected;
    refusalNativeCount = nativeCalls;
    int fd[2];
    assert(pipe(fd) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (!child) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(fd[0]);
        proofFd = fd[1];
        reports = 0;
        reason = value;
        refusalTarget = target;
        throwing = nativeError;
        if (target == 0x801733E0u)
            KnownNativeCpuCall<0x801733E0u>::Invoke(&cpu);
        else if (target == 0x801734E0u)
            KnownNativeCpuCall<0x801734E0u>::Invoke(&cpu);
        else
            KnownNativeCpuCall<0x80173400u>::Invoke(&cpu);
        _exit(90);
    }
    close(fd[1]);
    std::array<char, 3> proof{};
    std::size_t size = 0;
    for (;;) {
        const auto n = read(fd[0], proof.data() + size, proof.size() - size);
        assert(n >= 0);
        if (!n)
            break;
        size += static_cast<std::size_t>(n);
        assert(size < proof.size());
    }
    close(fd[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT && size == 2 && proof[0] == 'R' && proof[1] == 'A');
}
[[maybe_unused]] void Depth(float scale, float offset) {
    cpu.fpr[1].d = scale;
    cpu.fpr[2].d = offset;
    Save();
    expected = Capture();
    beforeNative = expected;
    const auto count = nativeCalls;
    const auto pointer = Memory::Contains(kGXDataPtrAddr, 4) ? Memory::Read32(kGXDataPtrAddr) : 0;
    if (pointer) {
        const std::array<std::uint32_t, 2> addresses{pointer + 0x55Cu, pointer + 0x560u};
        const std::array<float, 2> values{16777215.0f * offset, 1.0f + 16777215.0f * scale};
        bool valid = true;
        for (unsigned i = 0; i < 2; ++i) {
            if (!Memory::Contains(addresses[i], 4)) {
                valid = false;
                break;
            }
            ExpectedBytes(addresses[i], std::bit_cast<std::uint32_t>(values[i]), 4);
        }
        if (valid && Memory::Contains(pointer + 0x5FCu, 4))
            ExpectedBytes(pointer + 0x5FCu, Memory::Read32(pointer + 0x5FCu) | 0x10000000u, 4);
    }
    KnownNativeCpuCall<0x80173400u>::Invoke(&cpu);
    CheckCpu();
    Check(expected);
    assert(nativeCalls == count + 1 && std::strcmp(stage, "RMCP01_GX_SET_Z_SCALE_OFFSET") == 0);
    ++depth;
}
[[maybe_unused]] void Offset(std::int32_t x, std::int32_t y) {
    cpu.gpr[3] = std::bit_cast<std::uint32_t>(x);
    cpu.gpr[4] = std::bit_cast<std::uint32_t>(y);
    Save();
    expected = Capture();
    beforeNative = expected;
    const auto count = nativeCalls;
    const auto pointer = Memory::Contains(kGXDataPtrAddr, 4) ? Memory::Read32(kGXDataPtrAddr) : 0;
    if (pointer && Memory::Contains(pointer + 2u, 2))
        ExpectedBytes(pointer + 2u, 0, 2);
    KnownNativeCpuCall<0x801734E0u>::Invoke(&cpu);
    CheckCpu();
    Check(expected);
    assert(nativeCalls == count + 1 && std::strcmp(stage, "RMCP01_GX_SET_SCISSOR_BOX_OFFSET") == 0);
    ++offsets;
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
        if (!r.size)
            continue;
        assert(r.size <= page);
        auto* map = static_cast<std::uint8_t*>(mmap(nullptr, 3 * page, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        assert(map != MAP_FAILED);
        assert(mprotect(map + page, page, PROT_READ | PROT_WRITE) == 0);
        auto* bytes = map + 2 * page - r.size;
        std::memset(bytes, 0xa5, r.size);
        regions.push_back({r.base, r.size, 3 * page, map, bytes});
    }
}
std::uint8_t* HostPointer(std::uint32_t address) {
    for (const auto& r : regions)
        if (address >= r.base && std::uint64_t(address) < std::uint64_t(r.base) + r.size)
            return r.bytes + address - r.base;
    return nullptr;
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stages;
}
extern "C" void GXSetViewport(float a, float b, float c, float d, float e, float f) {
    CheckCpu();
    const std::array<float, 6> args{a, b, c, d, e, f};
    for (unsigned i = 0; i < 6; ++i) {
        assert(std::bit_cast<std::uint32_t>(args[i]) == std::bit_cast<std::uint32_t>(static_cast<float>(saved.fpr[i + 1].d)));
        assert(std::bit_cast<std::uint32_t>(g_viewportState[i]) == std::bit_cast<std::uint32_t>(args[i]));
    }
    ++nativeCalls;
}
extern "C" void GXSetZScaleOffset(float scale, float offset) {
    CheckCpu();
    Check(beforeNative);
    assert(std::bit_cast<std::uint32_t>(scale) == std::bit_cast<std::uint32_t>(static_cast<float>(saved.fpr[1].d)));
    assert(std::bit_cast<std::uint32_t>(offset) == std::bit_cast<std::uint32_t>(static_cast<float>(saved.fpr[2].d)));
    ++nativeCalls;
    if (throwing)
        throw std::runtime_error("native fixture");
}
extern "C" void GXSetScissorBoxOffset(std::int32_t x, std::int32_t y) {
    CheckCpu();
    Check(beforeNative);
    assert(x == std::bit_cast<std::int32_t>(saved.gpr[3]) && y == std::bit_cast<std::int32_t>(saved.gpr[4]));
    ++nativeCalls;
    if (throwing)
        throw std::runtime_error("native offset fixture");
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* value, std::uint32_t target, CpuContext* context) noexcept {
    assert(proofFd >= 0 && reports++ == 0 && context == &cpu && target == refusalTarget && std::strcmp(value, reason) == 0);
    CheckCpu();
    Check(expected);
    assert(nativeCalls == refusalNativeCount + (throwing ? 1u : 0u));
    assert(write(proofFd, "R", 1) == 1);
}
extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort() {
    if (proofFd >= 0) {
        assert(reports == 1);
        CheckCpu();
        assert(write(proofFd, "A", 1) == 1);
    }
    __real_abort();
}
int main() {
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x5b + 37 * i);
    Init();
    // Compare the production memory encoder against the pinned Gekko conversion,
    // including all exponent classes, arbitrary fractions and unaligned writes.
    for (unsigned exponent = 0; exponent < 2048; ++exponent)
        for (std::uint64_t fraction : {0ull, 1ull, 0x123456789abcdull, 0xfffffffffffffull})
            for (std::uint64_t sign : {0ull, 0x8000000000000000ull})
                for (unsigned alignment = 0; alignment < 4; ++alignment) {
                    const auto value = std::bit_cast<double>(sign | (std::uint64_t(exponent) << 52) | fraction);
                    expected = Capture();
                    ExpectedBytes(out + alignment, PinnedSingleBits(value), 4);
                    Memory::WriteFloat32(out + alignment, value);
                    Check(expected);
                }
    auto before = Capture();
    const auto oldStages = stages, oldNative = nativeCalls;
    KnownNativeCpuCall<0x801733B4u>::Invoke(nullptr);
    KnownNativeCpuCall<0x801733E0u>::Invoke(nullptr);
    KnownNativeCpuCall<0x80173400u>::Invoke(nullptr);
    KnownNativeCpuCall<0x801734E0u>::Invoke(nullptr);
    assert(stages == oldStages && nativeCalls == oldNative);
    Check(before);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    for (unsigned value = 0; value < 65536; ++value) {
        const double x = static_cast<double>(value) / 257.0;
        Set({x, -x, 128.0, 480.0, -0.0, 1.0});
        Get(out, Capture());
    }
    for (unsigned size = 0; size <= 24; ++size) {
        Init(size);
        Get(out, Capture());
    }
    Init(24, false);
    Get(out, Capture());
    for (auto table : {0x802A3F0Cu, 0x808B4C20u, 0xdeadbeefu})
        for (bool off : {false, true})
            for (unsigned flags = 0; flags < 256; ++flags) {
                Init();
                Memory::Write32(screen + 0x38, table);
                Memory::Write16(screen + 0x34, static_cast<std::uint16_t>(0xa500u | flags));
                Memory::Write32(off ? offList : onList, node);
                Memory::Write32(node + 4, 0);
                for (auto slot : {0x10u, 0x28u, 0x2Cu})
                    Memory::Write32(node + slot, screen);
                expected = Capture();
                if (table != 0xdeadbeefu && (off || (flags & 8u)))
                    ExpectedBytes(screen + 0x34, 0xa500u | flags | 64u, 2);
                Get(out, expected);
                // Same frame: a newly cleared flag must stay cleared. Native output still writes.
                Memory::Write16(screen + 0x34, 0);
                Get(0, Capture());
                ++g_gxFrameCount;
                expected = Capture();
                if (table != 0xdeadbeefu && off)
                    ExpectedBytes(screen + 0x34, 64u, 2);
                Get(0, expected);
            }
    // Complete screen membership, invalid pointer/alignment and finite traversal.
    for (unsigned size : {0u, 0x34u, 0x3cu, 0x3fu}) {
        Init(24, true, size);
        Memory::Write32(offList, node);
        Memory::Write32(node + 4, 0);
        Memory::Write32(node + 0x10, screen);
        Get(out, Capture());
    }
    for (auto pointer : {0u, screen + 1u, 0xdeadbeefu}) {
        Init();
        Memory::Write32(offList, node);
        Memory::Write32(node + 4, node);
        Memory::Write32(node + 0x10, pointer);
        Get(out, Capture());
    }
    // 65 linked nodes: only 64 may be visited, with the final screen unchanged.
    Init();
    Memory::Write32(screen + 0x38, 0x802A3F0C);
    Memory::Write16(screen + 0x34, 0);
    Memory::Write32(offList, node);
    for (unsigned i = 0; i < 65; ++i) {
        Memory::Write32(node + i * 32 + 4, i == 64 ? 0 : node + (i + 1) * 32);
        Memory::Write32(node + i * 32 + 16, i == 64 ? screen : 0);
    }
    Get(out, Capture());
    Init(4, false, 64, 0xfffffffcu);
    Get(0xfffffffcu, Capture());
    for (float s : {0.f, -0.f, 1.f, -1.f, 0.5f, 1e20f})
        for (float o : {0.f, -0.f, 1.f, -1.f, 0.5f, 1e20f}) {
            Init();
            Depth(s, o);
        }
    for (auto ptr : {0u, 0xdeadbeefu, gd + 1u}) {
        Init();
        Memory::Write32(kGXDataPtrAddr, ptr);
        Depth(1, 0);
    }
    for (auto limit : {0x55cu, 0x55fu, 0x560u, 0x564u, 0x5ffu, 0x600u}) {
        Memory::Config cfg;
        cfg.regions = {{"ptr", kGXDataPtrAddr, 4}, {"gd", gd, limit}};
        Memory::Init(cfg);
        Memory::Write32(kGXDataPtrAddr, gd);
        Depth(1, 0);
    }
    Init();
    for (int value = -342; value <= 1705; ++value) {
        Offset(value, 0);
        Offset(0, value);
    }
    for (auto x : {-342, -341, -1, 0, 1, 1704, 1705})
        for (auto y : {-342, -341, -1, 0, 1, 1704, 1705})
            Offset(x, y);
    for (auto pointer : {0u, 0xdeadbeefu, gd + 1u}) {
        Init();
        Memory::Write32(kGXDataPtrAddr, pointer);
        Offset(0, 0);
    }
    for (unsigned length : {2u, 3u, 4u}) {
        Memory::Config cfg;
        cfg.regions = {{"ptr", kGXDataPtrAddr, 4}, {"short-gd", gd, length}};
        Memory::Init(cfg);
        Memory::Write32(kGXDataPtrAddr, gd);
        Offset(0, 0);
    }
    {
        Memory::Config cfg;
        cfg.regions = {{"ptr", kGXDataPtrAddr, 4}, {"wrapped-flag", 0, 4}};
        Memory::Init(cfg);
        Memory::Write32(kGXDataPtrAddr, 0xfffffffeu);
        Offset(0, 0);
    }
    Init();
    for (int value : {-343, 1706, INT32_MIN, INT32_MAX})
        for (bool second : {false, true}) {
            cpu.gpr[3] = second ? 0u : std::bit_cast<std::uint32_t>(value);
            cpu.gpr[4] = second ? std::bit_cast<std::uint32_t>(value) : 0u;
            Refusal(0x801734E0u, "GX_SCISSOR_BOX_OFFSET_UNPROVEN_RANGE");
        }
    cpu.gpr[3] = cpu.gpr[4] = 0;
    Refusal(0x801734E0u, "GX_SCISSOR_BOX_OFFSET_NATIVE_EXCEPTION", true);
    Memory::Reset();
    Offset(0, 0);
    Init();
    Refusal(0x80173400u, "GX_Z_SCALE_OFFSET_NATIVE_EXCEPTION", true);
    Memory::Config broken;
    broken.regions = {{"offscreen", offList, 12}, {"short-node", node, 8}, {"output", out, 24}};
    Memory::Init(broken);
    Memory::Write32(offList, node);
    Memory::Write16(offList + 10, 0);
    Memory::Write32(node + 4, 0);
    ++g_gxFrameCount;
    cpu.gpr[3] = out;
    Refusal(0x801733E0u, "GX_GET_VIEWPORT_BYPASS_EXCEPTION");
    Memory::Reset();
    ++g_gxFrameCount;
    Get(out, Capture());
    std::printf("PASS: viewport setters=%u getters=%u depth=%u offsets=%u native refusal=1\n", setters, getters, depth, offsets);
#else
    Set({0, 0, 128, 128, 0, 1});
    Refusal(0x801733E0u, "GX_GET_VIEWPORT_REQUIRES_RENDERER");
    Refusal(0x80173400u, "GX_Z_SCALE_OFFSET_REQUIRES_RENDERER");
    Refusal(0x801734E0u, "GX_SCISSOR_BOX_OFFSET_REQUIRES_RENDERER");
    Memory::Reset();
    Refusal(0x801733E0u, "GX_GET_VIEWPORT_REQUIRES_RENDERER");
    std::puts("PASS: viewport headless setter preserved; diagnosed refusals=4");
#endif
    Memory::Reset();
}
