#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t address = 0x70001000u;
const char* stage = nullptr;
std::uint32_t calls = 0;
std::uint32_t calledTarget = 0;
GXIndTexMtxID matrixId = GX_ITM_OFF;
s8 exponent = 0;
std::array<float, 6> forwarded{};
GXIndTexStageID indStage = GX_INDTEXSTAGE0;
GXIndTexScale scaleS = GX_ITS_1;
GXIndTexScale scaleT = GX_ITS_1;
int reportPipe = -1;
const char* expectedReason = nullptr;
const char* expectedInvalidStage = nullptr;
CpuContext expectedCpu{};

struct HostRegion {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<HostRegion> hostRegions;

void ExpectStage(const char* expected) {
    assert(stage != nullptr && std::strcmp(stage, expected) == 0);
}

CpuContext MakeCpu(std::uint32_t r3, std::uint32_t r4, std::uint32_t r5) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = r3;
    cpu.gpr[4] = r4;
    cpu.gpr[5] = r5;
    return cpu;
}

void WriteMatrix(std::uint32_t location, const std::array<float, 6>& values) {
    for (std::uint32_t i = 0; i < values.size(); ++i) {
        const auto bits = std::bit_cast<std::uint32_t>(values[i]);
        // Independent byte encoding exercises the real Switch Memory::Read32
        // byte order rather than coupling the test to Memory::Write32.
        for (std::uint32_t byte = 0; byte < 4u; ++byte) {
            Memory::Write8(location + 4u * i + byte, bits >> (24u - 8u * byte));
        }
    }
}

template <std::uint32_t Target>
void InvokeAndCheck(CpuContext& cpu, const char* expectedStage) {
    std::array<unsigned char, sizeof(cpu)> before{};
    std::memcpy(before.data(), &cpu, sizeof(cpu));
    const auto previousCalls = calls;
    stage = nullptr;
    KnownNativeCpuCall<Target>::Invoke(&cpu);
    ExpectStage(expectedStage);
    assert(std::memcmp(before.data(), &cpu, sizeof(cpu)) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(calls == previousCalls + 1u && calledTarget == Target);
#else
    assert(calls == previousCalls);
#endif
}

void ExpectInvalid(CpuContext cpu, const char* reason, const char* invalidStage) {
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        reportPipe = descriptors[1];
        expectedReason = reason;
        expectedInvalidStage = invalidStage;
        std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
        KnownNativeCpuCall<0x80171814u>::Invoke(&cpu);
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
} // namespace

// Only the Horizon allocation boundary is replaced. The actual checked Switch
// Memory implementation and its big-endian reads/writes run in this test.
namespace GuestFlat {
bool IsActive() {
    return !hostRegions.empty();
}
void Initialize(const std::vector<RegionRequest>& regions) {
    hostRegions.clear();
    for (const auto& region : regions) {
        assert(region.backing == Backing::Owned);
        hostRegions.push_back({region.base, std::vector<std::uint8_t>(region.size)});
    }
}
std::uint8_t* HostPointer(std::uint32_t location) {
    for (auto& region : hostRegions) {
        if (location >= region.base &&
            std::uint64_t(location) < std::uint64_t(region.base) + region.bytes.size()) {
            return region.bytes.data() + (location - region.base);
        }
    }
    return nullptr;
}
void Shutdown() noexcept {
    hostRegions.clear();
}
} // namespace GuestFlat

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}

extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason,
    std::uint32_t target,
    CpuContext* cpu) noexcept {
    assert(reportPipe >= 0);
    assert(std::strcmp(reason, expectedReason) == 0);
    ExpectStage(expectedInvalidStage);
    assert(target == expectedCpu.gpr[4]);
    assert(std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0);
    assert(write(reportPipe, "R", 1) == 1);
}

extern "C" void GXSetIndTexMtx(GXIndTexMtxID id, const void* offset, s8 scaleExp) {
    assert(reportPipe < 0);
    ExpectStage("RMCP01_GX_SET_IND_TEX_MTX");
    ++calls;
    calledTarget = 0x80171814u;
    matrixId = id;
    exponent = scaleExp;
    std::memcpy(forwarded.data(), offset, sizeof(forwarded));
}

extern "C" void GXSetIndTexCoordScale(
    GXIndTexStageID value,
    GXIndTexScale s,
    GXIndTexScale t) {
    ExpectStage("RMCP01_GX_SET_IND_TEX_COORD_SCALE");
    ++calls;
    calledTarget = 0x80171968u;
    indStage = value;
    scaleS = s;
    scaleT = t;
}

int main() {
    static_assert(KnownNativeCpuCall<0x80171814u>::kAvailable);
    static_assert(KnownNativeCpuCall<0x80171968u>::kAvailable);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    KnownNativeCpuCall<0x80171814u>::Invoke(nullptr);
    KnownNativeCpuCall<0x80171968u>::Invoke(nullptr);
    assert(calls == 0u && stage == nullptr);

    ExpectInvalid(
        MakeCpu(1u, address, 1u),
        "GX_SET_IND_TEX_MTX_INVALID_MATRIX",
        "RMCP01_GX_SET_IND_TEX_MTX_INVALID_MATRIX");
    Memory::Config config;
    config.regions.push_back({"matrix-test", address, 64u});
    config.regions.push_back({"wrap-test", 0xfffffff0u, 16u});
    Memory::Init(config);

    const std::array<float, 6> values{0.f, -0.f, 0.5f, -0.75f, 1.25f, -2.f};
    for (auto location : {address, address + 1u, address + 40u}) {
        WriteMatrix(location, values);
        std::array<std::uint8_t, 24> bytes{};
        std::memcpy(bytes.data(), Memory::GetPointer(location, bytes.size()), bytes.size());
        for (auto id : {0u, 1u, 2u, 3u, 5u, 6u, 7u, 9u, 10u, 11u}) {
            for (auto scale : {0u, 1u, 17u, 0x7fu, 0x80u, 0xffu, 0x100u, 0xffffffffu}) {
                auto cpu = MakeCpu(id, location, scale);
                InvokeAndCheck<0x80171814u>(cpu, "RMCP01_GX_SET_IND_TEX_MTX");
                assert(std::memcmp(bytes.data(), Memory::GetPointer(location, bytes.size()),
                                   bytes.size()) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
                assert(matrixId == static_cast<GXIndTexMtxID>(id));
                assert(exponent == static_cast<s8>(scale));
                assert(std::memcmp(forwarded.data(), values.data(), sizeof(values)) == 0);
#endif
            }
        }
    }
    for (auto location : {0u, address - 4u, address + 41u, address + 64u, 0xfffffff0u}) {
        ExpectInvalid(
            MakeCpu(1u, location, 1u),
            "GX_SET_IND_TEX_MTX_INVALID_MATRIX",
            "RMCP01_GX_SET_IND_TEX_MTX_INVALID_MATRIX");
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
        for (auto invalid : {std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<float>::infinity(),
                             -std::numeric_limits<float>::infinity(),
                             std::numeric_limits<float>::max(),
                             2097152.f, std::nextafter(-2097152.f, -INFINITY)}) {
            auto matrix = values;
            matrix[i] = invalid;
            WriteMatrix(address, matrix);
            ExpectInvalid(
                MakeCpu(1u, address, 1u),
                "GX_SET_IND_TEX_MTX_INVALID_COEFFICIENT",
                "RMCP01_GX_SET_IND_TEX_MTX_INVALID_COEFFICIENT");
        }
    }
    auto limits = values;
    limits[0] = -2097152.f;
    limits[1] = std::nextafter(2097152.f, 0.f);
    WriteMatrix(address, limits);
    auto cpu = MakeCpu(1u, address, 1u);
    InvokeAndCheck<0x80171814u>(cpu, "RMCP01_GX_SET_IND_TEX_MTX");
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(std::memcmp(forwarded.data(), limits.data(), sizeof(limits)) == 0);
#endif

    // Legal scale enums occupy 0..8. Test the complete cross product and all
    // four stages so stage choice or an S/T argument swap cannot hide.
    for (std::uint32_t value = 0; value < 4u; ++value) {
        for (std::uint32_t s = 0; s <= 8u; ++s) {
            for (std::uint32_t t = 0; t <= 8u; ++t) {
                cpu = MakeCpu(value, s, t);
                InvokeAndCheck<0x80171968u>(cpu, "RMCP01_GX_SET_IND_TEX_COORD_SCALE");
#if MKW_LOCAL_RENDERED_FAST_TRACK
                assert(indStage == static_cast<GXIndTexStageID>(value));
                assert(scaleS == static_cast<GXIndTexScale>(s));
                assert(scaleT == static_cast<GXIndTexScale>(t));
#endif
            }
        }
    }
    const auto previousCalls = calls;
    const auto previousStage = stage;
    KnownNativeCpuCall<0x80171814u>::Invoke(nullptr);
    KnownNativeCpuCall<0x80171968u>::Invoke(nullptr);
    assert(calls == previousCalls && stage == previousStage);
    Memory::Reset();
}
