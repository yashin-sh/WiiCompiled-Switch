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
constexpr std::uint32_t address = 0x70002000u;
const char* stage = nullptr;
std::uint32_t frameCalls = 0;
std::uint32_t gxCalls = 0;
bool frameActive = false;
bool replaceColorAtEnsure = false;
std::uint32_t ensureColorAddress = address;
GXChannelID channel = GX_COLOR0;
GXColor forwarded{};
int reportPipe = -1;
std::uint32_t expectedFrameCalls = 0;
CpuContext expectedCpu{};
struct HostRegion {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<HostRegion> regions;

void ExpectStage(const char* expected) {
    assert(stage && std::strcmp(stage, expected) == 0);
}
CpuContext MakeCpu(std::uint32_t id, std::uint32_t pointer) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = id;
    cpu.gpr[4] = pointer;
    return cpu;
}
void WriteColor(std::uint32_t pointer, std::uint32_t value) {
    // Independent big-endian bytes exercise actual Switch Memory::Read32.
    for (std::uint32_t i = 0; i < 4u; ++i) {
        Memory::Write8(pointer + i, value >> (24u - 8u * i));
    }
}
void InvokeAndCheck(CpuContext& cpu, std::uint32_t value) {
    std::array<unsigned char, sizeof(cpu)> before{};
    std::memcpy(before.data(), &cpu, sizeof(cpu));
    const auto previousFrames = frameCalls;
    const auto previousGx = gxCalls;
    stage = nullptr;
    KnownNativeCpuCall<0x8017039Cu>::Invoke(&cpu);
    ExpectStage("RMCP01_GX_SET_CHAN_AMB_COLOR");
    assert(std::memcmp(before.data(), &cpu, sizeof(cpu)) == 0);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(frameCalls == previousFrames + 1u && gxCalls == previousGx + 1u);
    assert(channel == static_cast<GXChannelID>(cpu.gpr[3]));
    assert(forwarded.r == ((value >> 24) & 0xffu));
    assert(forwarded.g == ((value >> 16) & 0xffu));
    assert(forwarded.b == ((value >> 8) & 0xffu));
    assert(forwarded.a == (value & 0xffu));
#else
    (void)value;
    assert(frameCalls == previousFrames && gxCalls == previousGx);
#endif
}
#if MKW_LOCAL_RENDERED_FAST_TRACK
void ExpectInvalid(std::uint32_t pointer) {
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        reportPipe = descriptors[1];
        auto cpu = MakeCpu(4u, pointer);
        std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
        expectedFrameCalls = frameCalls + 1u;
        KnownNativeCpuCall<0x8017039Cu>::Invoke(&cpu);
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
#endif
} // namespace

// Allocation-only Horizon seam. Memory::Contains/Read32 use the real Switch
// implementation, including range checks and big-endian conversion.
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
void EnsureAuroraFrameActive() {
    ExpectStage("RMCP01_GX_SET_CHAN_AMB_COLOR");
    ++frameCalls;
    frameActive = true;
    if (replaceColorAtEnsure) {
        // Deliberate test instrumentation distinguishes read-before-ensure
        // from the pinned ensure-before-read contract. The real helper does
        // not write guest colors.
        WriteColor(ensureColorAddress, 0xaabbccddu);
    }
}
extern "C" void GXSetChanAmbColor(GXChannelID id, GXColor color) {
    assert(reportPipe < 0 && frameActive);
    ExpectStage("RMCP01_GX_SET_CHAN_AMB_COLOR");
    ++gxCalls;
    channel = id;
    forwarded = color;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    assert(reportPipe >= 0);
    ExpectStage("RMCP01_GX_SET_CHAN_AMB_COLOR_INVALID_COLOR");
    assert(std::strcmp(reason, "GX_SET_CHAN_AMB_COLOR_INVALID_COLOR") == 0);
    assert(frameCalls == expectedFrameCalls);
    assert(target == expectedCpu.gpr[4]);
    assert(std::memcmp(cpu, &expectedCpu, sizeof(*cpu)) == 0);
    assert(write(reportPipe, "R", 1) == 1);
}

int main() {
    static_assert(KnownNativeCpuCall<0x8017039Cu>::kAvailable);
    static_assert(GX_COLOR0A0 == 4 && GX_COLOR1A1 == 5);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);
    KnownNativeCpuCall<0x8017039Cu>::Invoke(nullptr);
    assert(stage == nullptr && frameCalls == 0u && gxCalls == 0u);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    ExpectInvalid(address);
#else
    auto headless = MakeCpu(4u, 0u);
    InvokeAndCheck(headless, 0u); // Headless mode does not access guest memory.
#endif
    Memory::Config config;
    config.regions.push_back({"color-test", address, 64u});
    config.regions.push_back({"wrap-test", 0xfffffff0u, 16u});
    Memory::Init(config);
    for (auto pointer : {address, address + 1u, address + 60u, 0xfffffffcu}) {
        for (auto value : {0u, 0xffffffffu, 0x11223344u, 0x80ff017fu}) {
            WriteColor(pointer, value);
            std::array<std::uint8_t, 4> bytes{};
            std::memcpy(bytes.data(), Memory::GetPointer(pointer, 4u), 4u);
            for (std::uint32_t id = 0; id <= 5u; ++id) {
                for (auto active : {false, true}) {
                    frameActive = active;
                    auto cpu = MakeCpu(id, pointer);
                    InvokeAndCheck(cpu, value);
                    assert(std::memcmp(bytes.data(), Memory::GetPointer(pointer, 4u), 4u) == 0);
                }
            }
        }
    }
#if MKW_LOCAL_RENDERED_FAST_TRACK
    for (auto pointer : {0u, address - 4u, address + 61u, address + 64u, 0xfffffffdu}) {
        ExpectInvalid(pointer);
    }
    WriteColor(address, 0x01020304u);
    replaceColorAtEnsure = true;
    auto cpu = MakeCpu(4u, address);
    InvokeAndCheck(cpu, 0xaabbccddu);
    replaceColorAtEnsure = false;
#endif
    const auto previousFrames = frameCalls;
    const auto previousGx = gxCalls;
    const auto previousStage = stage;
    KnownNativeCpuCall<0x8017039Cu>::Invoke(nullptr);
    assert(stage == previousStage && frameCalls == previousFrames && gxCalls == previousGx);
    Memory::Reset();
}
