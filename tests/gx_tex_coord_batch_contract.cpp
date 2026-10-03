#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <sys/resource.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t scaleTarget = 0x80171180u;
constexpr std::uint32_t biasTarget = 0x801711fcu;
constexpr std::uint32_t genTarget = 0x8016e37cu;
constexpr std::uint32_t pointerSlot = 0x803886c8u;
constexpr std::uint32_t data = 0x70006000u;
constexpr std::uint32_t dataSize = 0x604u;
constexpr std::uint32_t enableOffset = 0x5e4u;
constexpr std::uint32_t sOffset = 0x108u;
constexpr std::uint32_t tOffset = 0x128u;
const char* stage = nullptr;
std::uint32_t nativeCalls = 0;
std::uint32_t expectedTarget = 0;
CpuContext expectedCpu{};
int reportPipe = -1;
const char* expectedReason = nullptr;
std::uint32_t expectedPreviousCalls = 0;

struct HostRegion {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
    bool operator==(const HostRegion&) const = default;
};
using Image = std::vector<HostRegion>;
Image regions;
Image beforeNative;

const char* StageFor(std::uint32_t target) {
    if (target == scaleTarget)
        return "RMCP01_GX_SET_TEX_COORD_SCALE_MANUALLY";
    if (target == biasTarget)
        return "RMCP01_GX_SET_TEX_COORD_BIAS";
    assert(target == genTarget);
    return "RMCP01_GX_SET_TEX_COORD_GEN2";
}
void ExpectStage(std::uint32_t target) {
    assert(stage && std::strcmp(stage, StageFor(target)) == 0);
}
CpuContext MakeCpu(std::uint32_t coord, std::uint32_t r4 = 0u,
                   std::uint32_t r5 = 0u, std::uint32_t r6 = 0u) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = coord;
    cpu.gpr[4] = r4;
    cpu.gpr[5] = r5;
    cpu.gpr[6] = r6;
    return cpu;
}
CpuContext GenCpu(std::uint32_t coord) {
    auto cpu = MakeCpu(coord, 1u, 4u, 60u);
    cpu.gpr[7] = 0u;
    cpu.gpr[8] = 125u;
    return cpu;
}

std::uint8_t* ImagePointer(Image& image, std::uint32_t address, std::size_t size) {
    for (auto& region : image) {
        if (address >= region.base &&
            std::uint64_t(address) + size <= std::uint64_t(region.base) + region.bytes.size()) {
            return region.bytes.data() + (address - region.base);
        }
    }
    assert(false && "expected field must be fully mapped");
    return nullptr;
}
std::uint32_t ImageWord(Image& image, std::uint32_t address) {
    const auto* bytes = ImagePointer(image, address, 4u);
    return (std::uint32_t(bytes[0]) << 24u) | (std::uint32_t(bytes[1]) << 16u) |
           (std::uint32_t(bytes[2]) << 8u) | bytes[3];
}
void SetImageWord(Image& image, std::uint32_t address, std::uint32_t word) {
    auto* bytes = ImagePointer(image, address, 4u);
    for (std::uint32_t i = 0; i < 4u; ++i)
        bytes[i] = word >> (24u - 8u * i);
}
void ZeroImageHalfword(Image& image, std::uint32_t address) {
    auto* bytes = ImagePointer(image, address, 2u);
    bytes[0] = bytes[1] = 0u;
}
void WriteWord(std::uint32_t address, std::uint32_t word) {
    // Independent raw BE bytes, never Memory::Write32, set test inputs.
    for (std::uint32_t i = 0; i < 4u; ++i)
        Memory::Write8(address + i, word >> (24u - 8u * i));
}

struct Mapping {
    std::uint32_t base;
    std::size_t size;
};
void InitMemory(std::initializer_list<Mapping> mappings) {
    Memory::Config config;
    for (const auto& mapping : mappings)
        config.regions.push_back({"test-allocation", mapping.base, mapping.size});
    Memory::Init(config);
    std::uint32_t wordIndex = 0u;
    for (auto& region : regions) {
        for (std::size_t i = 0; i < region.bytes.size(); i += 4u) {
            const auto word = 0xc47a915bu ^ (wordIndex++ * 0x01010303u);
            for (std::uint32_t j = 0; j < 4u && i + j < region.bytes.size(); ++j)
                region.bytes[i + j] = word >> (24u - 8u * j);
        }
    }
}
void InitFull(std::uint32_t gd = data, std::uint32_t size = dataSize) {
    InitMemory({{pointerSlot, 4u}, {gd, size}});
    WriteWord(pointerSlot, gd);
}

// Explicit completed prefixes encode the pin's swallowed failures and lack
// of rollback. Every other byte, including dirty state +0x5fc, is a canary.
Image ScaleImage(const CpuContext& cpu, std::uint32_t gd, std::uint32_t completed) {
    auto image = regions;
    const auto coord = cpu.gpr[3];
    if (completed >= 1u) {
        const auto word = ImageWord(image, gd + enableOffset);
        SetImageWord(image, gd + enableOffset,
                     (word & ~(1u << coord)) | (cpu.gpr[4] << coord));
    }
    if (completed >= 2u) {
        const auto address = gd + sOffset + coord * 4u;
        SetImageWord(image, address,
                     (ImageWord(image, address) & 0xffff0000u) | ((cpu.gpr[5] - 1u) & 0xffffu));
    }
    if (completed >= 3u) {
        const auto address = gd + tOffset + coord * 4u;
        SetImageWord(image, address,
                     (ImageWord(image, address) & 0xffff0000u) | ((cpu.gpr[6] - 1u) & 0xffffu));
    }
    if (completed >= 4u)
        ZeroImageHalfword(image, gd + 2u);
    return image;
}
Image BiasImage(const CpuContext& cpu, std::uint32_t gd, std::uint32_t completed) {
    auto image = regions;
    if (completed >= 1u) {
        const auto address = gd + sOffset + cpu.gpr[3] * 4u;
        SetImageWord(image, address,
                     (ImageWord(image, address) & 0xfffeffffu) | (cpu.gpr[4] << 16u));
    }
    if (completed >= 2u) {
        const auto address = gd + tOffset + cpu.gpr[3] * 4u;
        SetImageWord(image, address,
                     (ImageWord(image, address) & 0xfffeffffu) | (cpu.gpr[5] << 16u));
    }
    if (completed >= 3u)
        ZeroImageHalfword(image, gd + 2u);
    return image;
}

template <std::uint32_t Target>
void InvokeAndCheck(CpuContext& cpu, const Image& expected) {
    const auto expectedImage = expected;
    std::array<unsigned char, sizeof(cpu)> beforeCpu{};
    std::memcpy(beforeCpu.data(), &cpu, sizeof(cpu));
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    expectedTarget = Target;
    expectedPreviousCalls = nativeCalls;
    beforeNative = regions;
    stage = nullptr;
    KnownNativeCpuCall<Target>::Invoke(&cpu);
    ExpectStage(Target);
    assert(std::memcmp(beforeCpu.data(), &cpu, sizeof(cpu)) == 0);
    assert(regions == expectedImage);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == expectedPreviousCalls + 1u);
#else
    assert(nativeCalls == expectedPreviousCalls);
#endif
}

void CheckNative(std::uint32_t target) {
    assert(reportPipe < 0 && target == expectedTarget);
    ExpectStage(target);
    assert(nativeCalls == expectedPreviousCalls);
    assert(regions == beforeNative); // Native call must precede every mirror write.
    ++nativeCalls;
}

template <std::uint32_t Target>
void ExpectAbort(CpuContext cpu, const char* reason) {
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        reportPipe = descriptors[1];
        expectedReason = reason;
        expectedTarget = Target;
        expectedPreviousCalls = nativeCalls;
        std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
        beforeNative = regions;
        KnownNativeCpuCall<Target>::Invoke(&cpu);
        _exit(1);
    }
    close(descriptors[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    char reported = 0;
    assert(read(descriptors[0], &reported, 1) == 1 && reported == 'R');
    close(descriptors[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}

void CheckAbsentMirror() {
    auto scale = MakeCpu(7u, 1u, 0x12345678u, 0x9abcdef0u);
    InvokeAndCheck<scaleTarget>(scale, regions);
    auto bias = MakeCpu(7u, 1u, 0u);
    InvokeAndCheck<biasTarget>(bias, regions);
}
void CheckPartialMirror(std::uint32_t scaleSteps, std::uint32_t biasSteps,
                        std::uint32_t coord = 0u) {
    WriteWord(pointerSlot, data);
    if (Memory::Contains(data + enableOffset, 4u)) {
        // Bias is manually enabled, so a completed E read must reach +2.
        WriteWord(data + enableOffset, Memory::Read32(data + enableOffset) | (1u << coord));
    }
    auto scale = MakeCpu(coord, 1u, 0x12345678u, 0x9abcdef0u);
    const auto expectedScale = ScaleImage(scale, data, scaleSteps);
    InvokeAndCheck<scaleTarget>(scale, expectedScale);
    auto bias = MakeCpu(coord, 1u, 0u);
    const auto expectedBias = BiasImage(bias, data, biasSteps);
    InvokeAndCheck<biasTarget>(bias, expectedBias);
}
} // namespace

// Allocation-only host seam. Storage is independent for each synthetic
// mapping; the real Switch Memory implementation performs all field access.
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    regions.clear();
    for (const auto& request : requests)
        regions.push_back({request.base, std::vector<std::uint8_t>(request.size)});
}
std::uint8_t* HostPointer(std::uint32_t address) {
    for (auto& region : regions) {
        if (address >= region.base &&
            std::uint64_t(address) < std::uint64_t(region.base) + region.bytes.size()) {
            return region.bytes.data() + (address - region.base);
        }
    }
    return nullptr;
}
void Shutdown() noexcept {
    regions.clear();
}
} // namespace GuestFlat

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
extern "C" void GXSetTexCoordScaleManually(GXTexCoordID coord, GXBool enable, u16 ss, u16 ts) {
    CheckNative(scaleTarget);
    assert(coord == static_cast<GXTexCoordID>(expectedCpu.gpr[3]));
    assert(enable == (expectedCpu.gpr[4] == 1u));
    assert(ss == static_cast<u16>(expectedCpu.gpr[5]));
    assert(ts == static_cast<u16>(expectedCpu.gpr[6]));
}
extern "C" void GXSetTexCoordBias(GXTexCoordID coord, GXBool s, GXBool t) {
    CheckNative(biasTarget);
    assert(coord == static_cast<GXTexCoordID>(expectedCpu.gpr[3]));
    assert(s == (expectedCpu.gpr[4] == 1u) && t == (expectedCpu.gpr[5] == 1u));
}
extern "C" void GXSetTexCoordGen2(GXTexCoordID coord, GXTexGenType type, GXTexGenSrc src,
                                  u32 mtx, GXBool normalize, u32 postMtx) {
    CheckNative(genTarget);
    assert(coord == static_cast<GXTexCoordID>(expectedCpu.gpr[3]));
    assert(type == GX_TG_MTX2x4 && src == GX_TG_TEX0 && mtx == GX_IDENTITY);
    assert(normalize == GX_FALSE && postMtx == GX_PTIDENTITY);
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(reportPipe >= 0 && target == expectedTarget);
    ExpectStage(target);
    assert(std::strcmp(reason, expectedReason) == 0);
    assert(std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0);
    assert(nativeCalls == expectedPreviousCalls && regions == beforeNative);
    assert(write(reportPipe, "R", 1) == 1);
}

int main() {
    static_assert(std::is_same_v<GXBool, bool>);
    static_assert(KnownNativeCpuCall<scaleTarget>::kAvailable);
    static_assert(KnownNativeCpuCall<biasTarget>::kAvailable);
    static_assert(KnownNativeCpuCall<genTarget>::kAvailable);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    KnownNativeCpuCall<scaleTarget>::Invoke(nullptr);
    KnownNativeCpuCall<biasTarget>::Invoke(nullptr);
    KnownNativeCpuCall<genTarget>::Invoke(nullptr);
    assert(stage == nullptr && nativeCalls == 0u);

    const std::array<std::array<std::uint32_t, 2>, 6> sizes{{{0u, 0u}, {1u, 1u}, {0xffffu, 0x10000u}, {0x10000u, 0xffffu}, {0x12345678u, 0x9abcdef0u}, {0xffffffffu, 0xffffffffu}}};
    for (std::uint32_t coord = 0; coord < 8u; ++coord) {
        for (std::uint32_t enabled = 0; enabled < 2u; ++enabled) {
            for (const auto& size : sizes) {
                InitFull();
                auto cpu = MakeCpu(coord, enabled, size[0], size[1]);
                const auto expected = ScaleImage(cpu, data, enabled ? 4u : 1u);
                InvokeAndCheck<scaleTarget>(cpu, expected);
            }
        }
        for (std::uint32_t manual = 0; manual < 2u; ++manual) {
            for (std::uint32_t s = 0; s < 2u; ++s) {
                for (std::uint32_t t = 0; t < 2u; ++t) {
                    InitFull();
                    WriteWord(data + enableOffset,
                              (Memory::Read32(data + enableOffset) & ~(1u << coord)) |
                                  (manual << coord));
                    auto cpu = MakeCpu(coord, s, t);
                    const auto expected = BiasImage(cpu, data, manual ? 3u : 2u);
                    InvokeAndCheck<biasTarget>(cpu, expected);
                }
            }
        }
        InitFull();
        auto cpu = GenCpu(coord);
        InvokeAndCheck<genTarget>(cpu, regions); // Gen2 has no guest mirror.
    }

    // No mirror backing is required for a valid native invocation.
    Memory::Reset();
    CheckAbsentMirror();
    InitMemory({{data, dataSize}}); // Absent GXData pointer slot.
    CheckAbsentMirror();
    InitMemory({{pointerSlot, 3u}, {data, dataSize}}); // Truncated pointer slot.
    CheckAbsentMirror();
    InitFull();
    WriteWord(pointerSlot, 0u);
    CheckAbsentMirror();
    WriteWord(pointerSlot, data + 0x10000u);
    CheckAbsentMirror();

    // Full structure validation would erase these pinned partial effects.
    InitFull(data, enableOffset + 3u); // E read fails; bias S/T persist.
    CheckPartialMirror(0u, 2u);
    InitMemory({{pointerSlot, 4u}, {data, 4u}, {data + enableOffset, 4u}});
    CheckPartialMirror(1u, 0u); // Scale E persists before an absent S.
    InitMemory({{pointerSlot, 4u}, {data, 4u}, {data + enableOffset, 4u}, {data + sOffset, 4u}});
    CheckPartialMirror(2u, 1u); // S persists before an absent T.
    InitMemory({{pointerSlot, 4u}, {data, 4u}, {data + enableOffset, 4u}, {data + sOffset, 3u}, {data + tOffset, 4u}});
    CheckPartialMirror(1u, 0u); // Truncated S must leave mapped T untouched.
    InitMemory({{pointerSlot, 4u}, {data, 3u}, {data + enableOffset, 4u}, {data + sOffset, 4u}, {data + tOffset, 4u}});
    CheckPartialMirror(3u, 2u); // A one-byte +2 tail cannot be half-written.
    InitMemory({{pointerSlot, 4u}, {data + 2u, 2u}, {data + enableOffset, 4u}, {data + sOffset + 28u, 4u}, {data + tOffset + 28u, 4u}});
    CheckPartialMirror(4u, 3u, 7u); // Exact ends; no contiguous GXData object.

    // A complete E word ending at 2^32 is valid. Overflowing additions may
    // not wrap into low mapped canaries, even though the pin used u32 maths.
    constexpr std::uint32_t nearEnd = 0xfffffa18u;
    InitFull(nearEnd, enableOffset + 4u);
    auto scale = MakeCpu(7u, 1u, 0u, 0xffffffffu);
    InvokeAndCheck<scaleTarget>(scale, ScaleImage(scale, nearEnd, 4u));
    auto bias = MakeCpu(7u, 1u, 1u);
    InvokeAndCheck<biasTarget>(bias, BiasImage(bias, nearEnd, 3u));
    InitMemory({{pointerSlot, 4u}, {0xfffffff0u, 16u}, {0x000000f8u, 0x600u}});
    WriteWord(pointerSlot, 0xfffffff0u);
    CheckAbsentMirror();
    constexpr std::uint32_t beforeTOverflow = 0xfffffed8u;
    InitMemory({{pointerSlot, 4u}, {beforeTOverflow, 0x128u}, {0u, 0x600u}});
    WriteWord(pointerSlot, beforeTOverflow);
    scale = MakeCpu(0u, 1u, 1u, 1u);
    InvokeAndCheck<scaleTarget>(scale, regions); // E addition overflows first.
    bias = MakeCpu(0u, 1u, 0u);
    InvokeAndCheck<biasTarget>(bias, BiasImage(bias, beforeTOverflow, 1u));
    constexpr std::uint32_t beforeEOverflow = 0xfffffeb0u;
    InitMemory({{pointerSlot, 4u}, {beforeEOverflow, 0x150u}, {0u, 0x600u}});
    WriteWord(pointerSlot, beforeEOverflow);
    InvokeAndCheck<scaleTarget>(scale, regions);
    InvokeAndCheck<biasTarget>(bias, BiasImage(bias, beforeEOverflow, 2u));
    InitFull(data + 1u); // Unaligned GXData remains legal checked storage.
    scale = MakeCpu(3u, 1u, 0xffffu, 0x10000u);
    InvokeAndCheck<scaleTarget>(scale, ScaleImage(scale, data + 1u, 4u));

    // Guards must report and abort before GX casts/indexing or any mirror.
    InitFull();
    for (auto invalid : {8u, 15u, 31u, 32u, 255u, 0xffffffffu}) {
        ExpectAbort<scaleTarget>(MakeCpu(invalid, 1u),
                                 "GX_SET_TEX_COORD_SCALE_MANUALLY_UNPROVEN_ARGS");
        ExpectAbort<biasTarget>(MakeCpu(invalid, 1u, 1u),
                                "GX_SET_TEX_COORD_BIAS_UNPROVEN_ARGS");
        ExpectAbort<genTarget>(GenCpu(invalid), "GX_SET_TEX_COORD_GEN2_UNPROVEN_ARGS");
    }
    for (auto invalid : {2u, 0xffffffffu}) {
        ExpectAbort<scaleTarget>(MakeCpu(0u, invalid),
                                 "GX_SET_TEX_COORD_SCALE_MANUALLY_UNPROVEN_ARGS");
        ExpectAbort<biasTarget>(MakeCpu(0u, invalid, 1u),
                                "GX_SET_TEX_COORD_BIAS_UNPROVEN_ARGS");
        ExpectAbort<biasTarget>(MakeCpu(0u, 1u, invalid),
                                "GX_SET_TEX_COORD_BIAS_UNPROVEN_ARGS");
    }
    for (std::uint32_t reg = 4u; reg <= 8u; ++reg) {
        auto cpu = GenCpu(7u);
        cpu.gpr[reg] ^= 1u;
        ExpectAbort<genTarget>(cpu, "GX_SET_TEX_COORD_GEN2_UNPROVEN_ARGS");
    }
    const auto previousCalls = nativeCalls;
    const auto previousStage = stage;
    const auto previousImage = regions;
    KnownNativeCpuCall<scaleTarget>::Invoke(nullptr);
    KnownNativeCpuCall<biasTarget>::Invoke(nullptr);
    KnownNativeCpuCall<genTarget>::Invoke(nullptr);
    assert(stage == previousStage && nativeCalls == previousCalls && regions == previousImage);
    Memory::Reset();
}
