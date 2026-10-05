#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
constexpr std::uint32_t object = 0x80384170u;
constexpr std::uint32_t data = 0x802a2b60u;
constexpr bool strict = MKW_STRICT_GX_TEXTURE_OBSERVED_TUPLES;
struct Region {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<Region> regions;
std::vector<std::vector<std::uint8_t>> expectedMemory;
CpuContext expectedCpu{};
const char* stage = nullptr;
const char* expectedReason = nullptr;
int reportPipe = -1;
unsigned nativeCalls = 0;
GXTexObj* initializedHost = nullptr;
bool throwLod = false;
bool trackDataRead = false;
bool dataRead = false;
unsigned validCases = 0;
unsigned refusals = 0;

void CheckStage(const char* expected) {
    assert(stage && std::strcmp(stage, expected) == 0);
}
void Snapshot(const CpuContext& cpu) {
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    expectedMemory.clear();
    for (const auto& r : regions)
        expectedMemory.push_back(r.bytes);
}
void CheckSnapshot(const CpuContext& cpu) {
    assert(std::memcmp(&cpu, &expectedCpu, sizeof(cpu)) == 0);
    assert(regions.size() == expectedMemory.size());
    for (std::size_t i = 0; i < regions.size(); ++i)
        assert(regions[i].bytes == expectedMemory[i]);
}
CpuContext LodCpu() {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = object;
    for (unsigned i = 4; i <= 8; ++i)
        cpu.gpr[i] = 0;
    cpu.fpr[1].d = cpu.fpr[2].d = cpu.fpr[3].d = 0.0;
    return cpu;
}
void InitMemory(std::uint32_t descriptorBytes = 64) {
    Memory::Config config;
    config.regions.push_back({"descriptor-and-sentinels", object - 16u, descriptorBytes});
    config.regions.push_back({"data", data, 64});
    Memory::Init(config);
    for (auto& r : regions)
        std::fill(r.bytes.begin(), r.bytes.end(), 0xa5);
}
void WriteDescriptor(std::uint32_t width, std::uint32_t height) {
    const std::array<std::uint32_t, 8> words{
        0x95u, 0u, (width - 1u) | ((height - 1u) << 10u) | 0x00600000u,
        0x0001515bu, 0u, 22u, 0u,
        ((((width + 3u) / 4u) * ((height + 3u) / 4u)) << 16u) | 0x302u};
    for (unsigned i = 0; i < words.size(); ++i)
        Memory::Write32(object + 4u * i, words[i]);
}
void InitHost() {
    auto cpu = LodCpu();
    cpu.gpr[4] = data;
    cpu.gpr[5] = cpu.gpr[6] = 4;
    cpu.gpr[7] = 22;
    cpu.gpr[8] = cpu.gpr[9] = 1;
    cpu.gpr[10] = 0;
    const auto before = cpu;
    nativeCalls = 0;
    KnownNativeCpuCall<0x801707f8u>::Invoke(&cpu);
    assert(std::memcmp(&before, &cpu, sizeof(cpu)) == 0);
    assert(Memory::Read32(object) == 0x95u);
    assert(Memory::Read32(object + 8u) == 0x00600c03u);
    assert(Memory::Read32(object + 12u) == 0x0001515bu);
    assert(Memory::Read32(object + 20u) == 22u);
    assert(Memory::Read32(object + 28u) == 0x00010302u);
    assert(nativeCalls == (MKW_LOCAL_RENDERED_FAST_TRACK ? 1u : 0u));
}
void ExpectAbort(CpuContext cpu, const char* reason) {
    Snapshot(cpu);
    int pipeFd[2]{};
    assert(pipe(pipeFd) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        assert(prctl(PR_SET_DUMPABLE, 0) == 0);
        close(pipeFd[0]);
        reportPipe = pipeFd[1];
        expectedReason = reason;
        nativeCalls = 0;
        KnownNativeCpuCall<0x80170a4cu>::Invoke(&cpu);
        _exit(92);
    }
    close(pipeFd[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    char result = 0;
    assert(read(pipeFd[0], &result, 1) == 1 && result == 'R');
    close(pipeFd[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    CheckSnapshot(cpu);
    ++refusals;
}
void CheckLod(std::uint32_t width, std::uint32_t height) {
    WriteDescriptor(width, height);
    auto cpu = LodCpu();
    Snapshot(cpu);
    const auto base = object - regions[0].base;
    // Audited zero/nearest fixture: clear filters, set inverted edge bit.
    expectedMemory[0][base] = 0;
    expectedMemory[0][base + 1u] = 0;
    expectedMemory[0][base + 2u] = 1;
    expectedMemory[0][base + 3u] = 5;
    nativeCalls = 0;
    dataRead = false;
    trackDataRead = true;
    KnownNativeCpuCall<0x80170a4cu>::Invoke(&cpu);
    trackDataRead = false;
    CheckStage("RMCP01_GX_INIT_TEX_OBJ_LOD");
    CheckSnapshot(cpu);
    assert(!dataRead);
    assert(nativeCalls == (MKW_LOCAL_RENDERED_FAST_TRACK ? 1u : 0u));
#if MKW_LOCAL_RENDERED_FAST_TRACK
    std::ifstream file("sdmc:/switch/WiiCompiled-Switch/fast-track-gx-init-tex-obj-lod.txt");
    const std::string status(std::istreambuf_iterator<char>(file), {});
    assert(status.find("status=lod-pass\n") != std::string::npos);
    assert(status.find("guest_word0=0x00000105\n") != std::string::npos);
#endif
    ++validCases;
}
} // namespace

// Allocation seam only; range checks and endian reads/writes are the real Switch Memory.
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    regions.clear();
    for (const auto& r : requests)
        regions.push_back({r.base, std::vector<std::uint8_t>(r.size)});
}
std::uint8_t* HostPointer(std::uint32_t pointer) {
    if (trackDataRead && pointer >= data && pointer < data + 64u)
        dataRead = true;
    for (auto& r : regions)
        if (pointer >= r.base && std::uint64_t(pointer) < std::uint64_t(r.base) + r.bytes.size())
            return r.bytes.data() + (pointer - r.base);
    return nullptr;
}
void Shutdown() noexcept {
    regions.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
void* GuestToHostPtr(std::uint32_t pointer, std::size_t length) {
    assert(!trackDataRead); // LOD updates mode words, never resolves a texture payload.
    return Memory::GetPointer(pointer, length);
}
extern "C" void GXInitTexObj(GXTexObj* obj, const void* ptr, u16 width, u16 height,
                             GXTexFmt format, GXTexWrapMode s, GXTexWrapMode t, GXBool mip) {
    CheckStage("RMCP01_GX_INIT_TEX_OBJ");
    assert(nativeCalls++ == 0);
    assert(ptr == Memory::GetPointer(data, 1));
    assert(width == 4 && height == 4 && format == GX_TF_Z24X8);
    assert(s == GX_REPEAT && t == GX_REPEAT && mip == GX_FALSE);
    initializedHost = obj;
}
extern "C" void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter min, GXTexFilter mag,
                                float minLod, float maxLod, float bias, GXBool clamp,
                                GXBool edge, GXAnisotropy aniso) {
    if (reportPipe >= 0 && !throwLod)
        _exit(91); // Unexpected native forwarding cannot fake the required diagnosed SIGABRT.
    CheckStage("RMCP01_GX_INIT_TEX_OBJ_LOD");
    assert(nativeCalls++ == 0 && obj == initializedHost);
    assert(min == GX_NEAR && mag == GX_NEAR);
    assert(minLod == 0.f && maxLod == 0.f && bias == 0.f);
    assert(clamp == GX_FALSE && edge == GX_FALSE && aniso == GX_ANISO_1);
    assert(Memory::Read32(object) == 0x95u); // Native call precedes guest writeback.
    if (throwLod)
        throw std::runtime_error("test native LOD failure");
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(reportPipe >= 0 && target == 0x80170a4cu);
    assert(std::strcmp(reason, expectedReason) == 0);
    CheckStage("RMCP01_GX_INIT_TEX_OBJ_LOD");
    CheckSnapshot(*cpu);
    assert(nativeCalls == (throwLod ? 1u : 0u));
    assert(write(reportPipe, "R", 1) == 1);
}
int main() {
    static_assert(GX_TF_Z24X8 == 22);
    static_assert(KnownNativeCpuCall<0x80170a4cu>::kAvailable);
    KnownNativeCpuCall<0x80170a4cu>::Invoke(nullptr);
    assert(stage == nullptr && nativeCalls == 0);
    ExpectAbort(LodCpu(), "GX_INIT_TEX_OBJ_LOD_GUEST_UNMAPPED");
    InitMemory(47); // One byte short of the complete object range.
    ExpectAbort(LodCpu(), "GX_INIT_TEX_OBJ_LOD_GUEST_UNMAPPED");
    InitMemory();
    WriteDescriptor(4, 4);
    if (strict) {
        ExpectAbort(LodCpu(), "GX_INIT_TEX_OBJ_LOD_UNPROVEN_TUPLE");
    } else {
#if MKW_LOCAL_RENDERED_FAST_TRACK
        ExpectAbort(LodCpu(), "GX_INIT_TEX_OBJ_LOD_HOST_OBJ_MISSING");
#endif
        InitHost();
        CheckLod(4, 4);
        for (auto width : {1u, 3u, 4u, 5u, 31u, 32u, 33u, 1024u})
            for (auto height : {1u, 3u, 4u, 5u, 31u, 32u, 33u, 1024u})
                CheckLod(width, height);
        CheckLod(4, 4);
    }
    WriteDescriptor(4, 4);
    // Structural errors remain hard stops before native graphics or guest writes.
    const std::array<std::pair<unsigned, std::uint32_t>, 9> invalid{
        {{0, 0x97u}, {0, 0x9du}, {2, 0x00500c03u}, {3, 0xff01515bu}, {5, 0x116u}, {5, 0x26u}, {7, 0x00020302u}, {7, 0x00010202u}, {7, 0x00010300u}}};
    for (const auto& [word, value] : invalid) {
        const auto before = Memory::Read32(object + 4u * word);
        Memory::Write32(object + 4u * word, value);
        ExpectAbort(LodCpu(), "GX_INIT_TEX_OBJ_LOD_INVALID_DESCRIPTOR");
        Memory::Write32(object + 4u * word, before);
    }
    for (const auto reg : {4u, 5u, 6u, 7u, 8u}) {
        auto cpu = LodCpu();
        cpu.gpr[reg] = 0xffffffffu;
        ExpectAbort(cpu, "GX_INIT_TEX_OBJ_LOD_INVALID_ARGS");
    }
    for (const auto reg : {1u, 2u, 3u}) {
        auto cpu = LodCpu();
        cpu.fpr[reg].d = std::numeric_limits<double>::quiet_NaN();
        ExpectAbort(cpu, "GX_INIT_TEX_OBJ_LOD_INVALID_ARGS");
    }
#if MKW_LOCAL_RENDERED_FAST_TRACK
    if (!strict) {
        throwLod = true;
        ExpectAbort(LodCpu(), "GX_INIT_TEX_OBJ_LOD_HOST_EXCEPTION");
        throwLod = false;
    }
#endif
    const auto previousStage = stage;
    Snapshot(LodCpu());
    KnownNativeCpuCall<0x80170a4cu>::Invoke(nullptr);
    assert(stage == previousStage);
    CheckSnapshot(LodCpu());
    Memory::Reset();
    std::printf("PASS: depth LOD rendered=%u strict=%u valid=%u diagnosed-refusals=%u\n",
                MKW_LOCAL_RENDERED_FAST_TRACK, strict, validCases, refusals);
}
