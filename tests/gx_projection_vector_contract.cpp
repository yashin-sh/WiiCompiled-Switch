#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>
#include <array>
#include <bit>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr std::uint32_t base = 0x70000000u, input = base + 128, output = base + 256;
std::vector<std::uint8_t> bytes;
bool active = false;
unsigned calls = 0, expectedCalls = 0;
std::array<std::uint32_t, 16> nativeMatrix{};
GXProjectionType nativeType{};
CpuContext saved{};
std::vector<std::uint8_t> savedMemory;
const char* expectedReason = nullptr;
std::uint32_t expectedTarget = 0;
int proofFd = -1;
#if MKW_LOCAL_RENDERED_FAST_TRACK
std::array<std::uint32_t, 7> expectedVector{};
extern "C" float g_projectionVector[7];
#endif
[[maybe_unused]] void Word(std::vector<std::uint8_t>& target, std::uint32_t address, std::uint32_t bits) {
    for (unsigned i = 0; i < 4; ++i)
        target[address - base + i] = bits >> (24 - i * 8);
}
CpuContext Cpu(std::uint32_t address) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = address;
    return cpu;
}
void Invoke(std::uint32_t target, CpuContext* cpu) {
    if (target == 0x8017301c)
        KnownNativeCpuCall<0x8017301Cu>::Invoke(cpu);
    else if (target == 0x80173080)
        KnownNativeCpuCall<0x80173080u>::Invoke(cpu);
    else {
        assert(target == 0x801730cc);
        KnownNativeCpuCall<0x801730CCu>::Invoke(cpu);
    }
}
void Refusal(std::uint32_t target, std::uint32_t address, const char* reason) {
    auto cpu = Cpu(address);
    saved = cpu;
    savedMemory = bytes;
    expectedReason = reason;
    expectedTarget = target == 0x8017301c ? address : target;
    expectedCalls = calls;
#if MKW_LOCAL_RENDERED_FAST_TRACK
    std::memcpy(expectedVector.data(), g_projectionVector, 28);
#endif
    int pipeEnds[2];
    assert(pipe(pipeEnds) == 0);
    auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(pipeEnds[0]);
        proofFd = pipeEnds[1];
        const rlimit noCore{0, 0};
        assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
        Invoke(target, &cpu);
        _exit(9);
    }
    close(pipeEnds[1]);
    char proof = 0;
    assert(read(pipeEnds[0], &proof, 1) == 1 && proof == 'P');
    close(pipeEnds[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
}
#if MKW_LOCAL_RENDERED_FAST_TRACK
void Get(const std::array<std::uint32_t, 7>& expected) {
    auto cpu = Cpu(output);
    auto before = cpu;
    auto memory = bytes;
    for (unsigned i = 0; i < 7; ++i)
        Word(memory, output + i * 4, expected[i]);
    const auto count = calls;
    Invoke(0x801730cc, &cpu);
    assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == memory && calls == count);
}
void SetVector(const std::array<std::uint32_t, 7>& vector, const std::array<std::uint32_t, 16>& expected, GXProjectionType type) {
    for (unsigned i = 0; i < 7; ++i)
        Word(bytes, input + i * 4, vector[i]);
    auto cpu = Cpu(input);
    auto before = cpu;
    auto memory = bytes;
    const auto count = calls;
    Invoke(0x80173080, &cpu);
    assert(calls == count + 1 && nativeMatrix == expected && nativeType == type);
    assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == memory);
    Get(vector);
}
#endif
} // namespace
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 4096);
    bytes.assign(4096, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == base ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(expectedReason && std::strcmp(reason, expectedReason) == 0 && target == expectedTarget);
    assert(std::memcmp(cpu, &saved, sizeof(saved)) == 0 && bytes == savedMemory && calls == expectedCalls);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(std::memcmp(expectedVector.data(), g_projectionVector, 28) == 0);
#endif
    assert(write(proofFd, "P", 1) == 1);
}
void GXSetProjection(const void* matrix, GXProjectionType type) {
    ++calls;
    std::memcpy(nativeMatrix.data(), matrix, sizeof(nativeMatrix));
    nativeType = type;
}
int main() {
    Memory::Config config;
    config.regions = {{"synthetic-projection", base, 4096}};
    Memory::Init(config);
    Invoke(0x8017301c, nullptr);
    Invoke(0x80173080, nullptr);
    Invoke(0x801730cc, nullptr);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    Get({0x3f800000, 0x3f800000, 0, 0x3f800000, 0, 0xbf800000, 0});
    std::array<std::uint32_t, 16> matrix;
    for (unsigned i = 0; i < 16; ++i) {
        matrix[i] = 0x3f800000 + i * 0x10000;
        Word(bytes, input + i * 4, matrix[i]);
    }
    for (auto type : {GX_PERSPECTIVE, GX_ORTHOGRAPHIC}) {
        auto cpu = Cpu(input);
        cpu.gpr[4] = type;
        auto before = cpu;
        auto memory = bytes;
        auto count = calls;
        Invoke(0x8017301c, &cpu);
        assert(calls == count + 1 && nativeMatrix == matrix && nativeType == type);
        assert(std::memcmp(&cpu, &before, sizeof(cpu)) == 0 && bytes == memory);
        // Independently specified matrix element order in the SDK projection vector.
        Get(type == GX_PERSPECTIVE ? std::array<std::uint32_t, 7>{0, 0x3f800000, 0x3f820000, 0x3f850000, 0x3f860000, 0x3f8a0000, 0x3f8b0000}
                                   : std::array<std::uint32_t, 7>{0x3f800000, 0x3f800000, 0x3f830000, 0x3f850000, 0x3f870000, 0x3f8a0000, 0x3f8b0000});
    }
    std::array<std::uint32_t, 7> vector{0, 0x40000000, 0x40400000, 0x40800000, 0x40a00000, 0x40c00000, 0x40e00000};
    SetVector(vector, {0x40000000, 0, 0x40400000, 0, 0, 0x40800000, 0x40a00000, 0, 0, 0, 0x40c00000, 0x40e00000, 0, 0, 0xbf800000, 0}, GX_PERSPECTIVE);
    vector[0] = 0x40000000; // Preserve a noncanonical nonzero type value in the shadow.
    SetVector(vector, {0x40000000, 0, 0, 0x40400000, 0, 0x40800000, 0, 0x40a00000, 0, 0, 0x40c00000, 0x40e00000, 0, 0, 0, 0x3f800000}, GX_ORTHOGRAPHIC);
    vector = {0x80000000, 0x80000000, 0x7fc01234, 0x7f800000, 0xff800000, 1, 0x80000001};
    SetVector(vector, {0x80000000, 0, 0x7fc01234, 0, 0, 0x7f800000, 0xff800000, 0, 0, 0, 1, 0x80000001, 0, 0, 0xbf800000, 0}, GX_PERSPECTIVE);
    for (auto address : {0u, base - 1, base + 4096 - 24, 0xfffffff0u}) {
        Refusal(0x801730cc, address, "GX_GET_PROJECTIONV_INVALID_OUTPUT");
        Refusal(0x80173080, address, "GX_SET_PROJECTIONV_INVALID_INPUT");
    }
    Refusal(0x8017301c, base + 4096 - 60, "GX_SET_PROJECTION_INVALID_MATRIX");
    Refusal(0x8017301c, 0xfffffff0u, "GX_SET_PROJECTION_INVALID_MATRIX");
    Memory::Reset();
    Refusal(0x801730cc, output, "GX_GET_PROJECTIONV_INVALID_OUTPUT");
#else
    Refusal(0x801730cc, output, "GX_GET_PROJECTIONV_REQUIRES_RENDERER");
    Refusal(0x80173080, input, "GX_SET_PROJECTIONV_REQUIRES_RENDERER");
#endif
    std::puts("PASS: projection matrix/vector state, exact big-endian output, CPU and canary preservation, and refusals before mutation");
}
