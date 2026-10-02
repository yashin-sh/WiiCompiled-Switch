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
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t address = 0x70003000u;
constexpr std::uint32_t shortAddress = address + 0x1000u;
constexpr std::uint32_t wrapAddress = 0xffffffc0u;
const char* stage = nullptr;
std::uint32_t gxCalls = 0;
std::uint32_t forwardedId = 0;
std::uint32_t forwardedType = 0;
std::array<std::uint32_t, 12> forwardedBits{};
int reportPipe = -1;
CpuContext expectedCpu{};
std::uint32_t expectedGxCalls = 0;

struct HostRegion {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<HostRegion> regions;

void ExpectStage(const char* expected) {
    assert(stage && std::strcmp(stage, expected) == 0);
}

CpuContext MakeCpu(std::uint32_t pointer, std::uint32_t id, std::uint32_t type) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = pointer;
    cpu.gpr[4] = id;
    cpu.gpr[5] = type;
    return cpu;
}

void WriteMatrix(std::uint32_t pointer,
                 const std::array<std::uint32_t, 12>& bits,
                 std::uint32_t count) {
    // Independent byte encoding exercises the real Switch Memory::Read32.
    // Coefficients are specified as bits so signed zero, subnormals and NaNs
    // cannot be changed by host floating-point comparisons or arithmetic.
    for (std::uint32_t i = 0; i < count; ++i) {
        for (std::uint32_t byte = 0; byte < 4u; ++byte) {
            Memory::Write8(pointer + 4u * i + byte, bits[i] >> (24u - 8u * byte));
        }
    }
}

void InvokeAndCheck(CpuContext& cpu, const std::array<std::uint32_t, 12>& expected) {
    std::array<unsigned char, sizeof(cpu)> before{};
    std::memcpy(before.data(), &cpu, sizeof(cpu));
    const auto previousCalls = gxCalls;
    stage = nullptr;
    KnownNativeCpuCall<0x80173234u>::Invoke(&cpu);
    ExpectStage("RMCP01_GX_LOAD_TEX_MTX_IMM");
    assert(std::memcmp(before.data(), &cpu, sizeof(cpu)) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(gxCalls == previousCalls + 1u);
    assert(forwardedId == cpu.gpr[4] && forwardedType == cpu.gpr[5]);
    assert(forwardedBits == expected);
#else
    (void)expected;
    assert(gxCalls == previousCalls);
#endif
}

void ExpectInvalid(std::uint32_t pointer, std::uint32_t type) {
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        reportPipe = descriptors[1];
        auto cpu = MakeCpu(pointer, GX_TEXMTX0, type);
        std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
        expectedGxCalls = gxCalls;
        KnownNativeCpuCall<0x80173234u>::Invoke(&cpu);
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

// Allocation-only Horizon seam. The actual Switch range checks and
// big-endian Memory::Read32 run unchanged, including unaligned addresses.
namespace GuestFlat {
bool IsActive() {
    return !regions.empty();
}
void Initialize(const std::vector<RegionRequest>& requests) {
    regions.clear();
    for (const auto& request : requests) {
        assert(request.backing == Backing::Owned);
        regions.push_back({request.base, std::vector<std::uint8_t>(request.size)});
    }
}
std::uint8_t* HostPointer(std::uint32_t pointer) {
    for (auto& region : regions) {
        if (pointer >= region.base &&
            std::uint64_t(pointer) < std::uint64_t(region.base) + region.bytes.size()) {
            return region.bytes.data() + (pointer - region.base);
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

extern "C" void GXLoadTexMtxImm(const void* matrix, u32 id, GXTexMtxType type) {
    assert(reportPipe < 0);
    ExpectStage("RMCP01_GX_LOAD_TEX_MTX_IMM");
    ++gxCalls;
    forwardedId = id;
    static_assert(sizeof(type) == sizeof(forwardedType));
    // Observe the exact representation at the bridge boundary. Rendered
    // cases use only legal enum values; Aurora retains its original CHECKs.
    std::memcpy(&forwardedType, &type, sizeof(type));
    std::memcpy(forwardedBits.data(), matrix, sizeof(forwardedBits));
}

extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(reportPipe >= 0);
    ExpectStage("RMCP01_GX_LOAD_TEX_MTX_IMM_INVALID_MATRIX");
    assert(std::strcmp(reason, "GX_LOAD_TEX_MTX_IMM_INVALID_MATRIX") == 0);
    assert(target == expectedCpu.gpr[3]);
    assert(std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0);
    assert(gxCalls == expectedGxCalls);
    assert(write(reportPipe, "R", 1) == 1);
}

int main() {
    static_assert(KnownNativeCpuCall<0x80173234u>::kAvailable);
    static_assert(GX_MTX3x4 == 0 && GX_MTX2x4 == 1);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    KnownNativeCpuCall<0x80173234u>::Invoke(nullptr);
    assert(stage == nullptr && gxCalls == 0u);
    ExpectInvalid(address, GX_MTX3x4);
    ExpectInvalid(address, GX_MTX2x4);

    Memory::Config config;
    config.regions.push_back({"matrix-test", address, 64u});
    config.regions.push_back({"eight-coefficients-only", shortAddress, 32u});
    config.regions.push_back({"wrap-test", wrapAddress, 64u});
    Memory::Init(config);

    // Twelve distinct encodings test all slots and their order, without
    // inventing coefficient range checks absent from the pinned wrapper.
    const std::array<std::uint32_t, 12> bits{
        0x00000000u, 0x80000000u, 0x3f000000u, 0xbf400000u,
        0x3fa00000u, 0xc0000000u, 0x00000001u, 0x007fffffu,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0x00800000u};
    const std::array<std::uint32_t, 12> eightBits{
        bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6], bits[7],
        0u, 0u, 0u, 0u};

#if MKW_LOCAL_RENDERED_FAST_TRACK
    constexpr std::array<std::uint32_t, 2> validTypes{0u, 1u};
#else
    // Headless makes no GXTexMtxType cast. Check the wrapper's eight-value
    // memory contract for every nonzero branch without an invalid enum cast.
    constexpr std::array<std::uint32_t, 4> validTypes{0u, 1u, 2u, 0xffffffffu};
#endif

    for (auto type : validTypes) {
        const auto count = type == 0u ? 12u : 8u;
        const auto& expected = type == 0u ? bits : eightBits;
        const auto endPointer = type == 0u ? address + 16u : address + 32u;
        const auto wrapPointer = type == 0u ? 0xffffffd0u : 0xffffffe0u;
        for (auto pointer : {address, address + 1u, endPointer, wrapPointer}) {
            WriteMatrix(pointer, bits, count);
            std::array<std::uint8_t, 48> bytes{};
            std::memcpy(bytes.data(), Memory::GetPointer(pointer, count * 4u), count * 4u);
            // Regular texture slots and identity, including the endpoints.
            for (auto id : {30u, 33u, 36u, 39u, 42u, 45u, 48u, 51u, 54u, 57u, 60u}) {
                auto cpu = MakeCpu(pointer, id, type);
                InvokeAndCheck(cpu, expected);
                assert(std::memcmp(bytes.data(), Memory::GetPointer(pointer, count * 4u),
                                   count * 4u) == 0);
            }
        }
    }

    // Aurora post matrices require 3x4; exercise the pinned legal form only.
    WriteMatrix(address, bits, 12u);
    for (std::uint32_t id = GX_PTTEXMTX0; id <= GX_PTTEXMTX19; id += 3u) {
        auto cpu = MakeCpu(address, id, GX_MTX3x4);
        InvokeAndCheck(cpu, bits);
    }
    auto identity = MakeCpu(address, GX_PTIDENTITY, GX_MTX3x4);
    InvokeAndCheck(identity, bits);

    // A mapped 32-byte object proves nonzero types do not read a third row.
    WriteMatrix(shortAddress, bits, 8u);
    for (auto type : validTypes) {
        if (type == 0u) {
            continue;
        }
        auto cpu = MakeCpu(shortAddress, GX_TEXMTX0, type);
        InvokeAndCheck(cpu, eightBits);
    }
    ExpectInvalid(shortAddress, GX_MTX3x4);

    for (auto type : {0u, 1u, 2u, 0xffffffffu}) {
        for (auto pointer : {0u, address - 4u, address + 64u, shortAddress + 1u}) {
            ExpectInvalid(pointer, type);
        }
        ExpectInvalid(type == 0u ? address + 17u : address + 33u, type);
        ExpectInvalid(type == 0u ? 0xffffffd1u : 0xffffffe1u, type);
        ExpectInvalid(0xfffffffcu, type);
    }
    const auto previousCalls = gxCalls;
    const auto previousStage = stage;
    KnownNativeCpuCall<0x80173234u>::Invoke(nullptr);
    assert(stage == previousStage && gxCalls == previousCalls);
    Memory::Reset();
    ExpectInvalid(address, GX_MTX3x4);
}
