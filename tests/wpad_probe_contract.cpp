#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "hle/controller_status_contract.h"
#include "memory_switch_slice.hpp"
#include "switch_input_hle_traits.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static_assert(KnownNativeCpuCall<0x801C0990u>::kAvailable);
static_assert(WpadContract::kChannelCount == 4);
static_assert(WpadContract::kErrorNoController == -1 && WpadContract::kErrorBadChannel == -6);
static_assert(WpadContract::kExtensionCore == 0);

namespace {
constexpr std::uint32_t base = 0x70000000u;
std::array<std::uint8_t, 64> backing;
bool active = false;
unsigned stages = 0;
unsigned cases = 0;
CpuContext* refusedCpu = nullptr;
CpuContext refusedExpected;
bool reported = false;
using namespace mkw::switch_input_hle;

void CheckState(bool initialized) {
    assert(g_wpad_initialized == initialized);
    assert(g_wpad_dpd_sensitivity == 7);
    assert(g_wpad_sync_device_callback == 0x12345678u);
    assert(g_pad_initialized == !initialized);
}

CpuContext Context(std::uint32_t channel, std::uint32_t output) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x39u + 23u * i);
    cpu.gpr[3] = channel;
    cpu.gpr[4] = output;
    return cpu;
}

void Invoke(std::uint32_t channel, std::uint32_t output, bool initialized) {
    g_wpad_initialized = initialized;
    g_wpad_dpd_sensitivity = 7;
    g_wpad_sync_device_callback = 0x12345678u;
    g_pad_initialized = !initialized;
    backing.fill(0xa5);
    auto cpu = Context(channel, output);
    CpuContext expected;
    std::memcpy(&expected, &cpu, sizeof(cpu));
    expected.gpr[3] = channel < 4 ? 0xffffffffu : 0xfffffffau;
    const auto previousStages = stages;
    KnownNativeCpuCall<0x801C0990u>::Invoke(&cpu);
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(stages == previousStages + 1);
    CheckState(initialized);
    for (std::size_t i = 0; i < backing.size(); ++i) {
        const bool written = channel < 4 && output && base + i >= output && base + i < output + 4;
        assert(backing[i] == (written ? 0 : 0xa5));
    }
    ++cases;
}

void Refuse(std::uint32_t output) {
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        g_wpad_initialized = false;
        g_wpad_dpd_sensitivity = 7;
        g_wpad_sync_device_callback = 0x12345678u;
        g_pad_initialized = true;
        backing.fill(0xa5);
        auto cpu = Context(0, output);
        std::memcpy(&refusedExpected, &cpu, sizeof(cpu));
        refusedCpu = &cpu;
        reported = false;
        KnownNativeCpuCall<0x801C0990u>::Invoke(&cpu);
        _exit(1); // Refusal must not return as success or absent controller.
    }
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 77);
    ++cases;
}
} // namespace

// Allocation and diagnostic seams only: exercise the production bridge and
// real Memory slice, with no host device or game-derived input.
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == backing.size());
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == base ? backing.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
}
} // namespace GuestFlat

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    if (std::strcmp(stage, "RMCP01_WPAD_PROBE") == 0)
        ++stages;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target,
                                                                  CpuContext* cpu) noexcept {
    assert(std::strcmp(reason, "WPAD_PROBE_TYPE_RANGE") == 0);
    assert(target == 0x801C0990u && cpu == refusedCpu);
    assert(std::memcmp(cpu, &refusedExpected, sizeof(*cpu)) == 0);
    reported = true;
}
extern "C" [[noreturn]] void __wrap_abort() noexcept {
    assert(reported && refusedCpu);
    assert(std::memcmp(refusedCpu, &refusedExpected, sizeof(*refusedCpu)) == 0);
    CheckState(false);
    for (const auto byte : backing)
        assert(byte == 0xa5);
    _exit(77);
}

int main() {
    const auto previousStages = stages;
    KnownNativeCpuCall<0x801C0990u>::Invoke(nullptr);
    assert(stages == previousStages);
    for (bool initialized : {false, true}) {
        for (unsigned channel = 0; channel < 4; ++channel)
            Invoke(channel, 0, initialized); // Probe does not require WPADInit or Memory.
        for (auto channel : {4u, 5u, 0xffffffffu})
            Invoke(channel, base, initialized); // Bad channel precedes pointer validation.
    }
    Refuse(base); // Non-null output needs mapped memory even before WPADInit.
    Memory::Config config;
    config.regions = {{"WPAD probe test", base, backing.size()}};
    Memory::Init(config);
    for (bool initialized : {false, true}) {
        for (unsigned channel = 0; channel < 4; ++channel) {
            Invoke(channel, 0, initialized);
            // Every 4-byte window, including unaligned and exact-end writes.
            for (unsigned offset = 0; offset <= 60; ++offset)
                Invoke(channel, base + offset, initialized);
        }
        for (auto channel : {4u, 5u, 0xffffffffu})
            for (auto output : {0u, base + 1, base + 63, 0xffffffffu})
                Invoke(channel, output, initialized);
    }
    for (auto output : {base - 1, base + 61, base + 62, base + 63, base + 64, 0xfffffffeu, 0xffffffffu})
        Refuse(output);
    Memory::Reset();
    Invoke(0, 0, false);
    Refuse(base + 1);
    std::printf("PASS: WPADProbe %u CPU/memory/state cases; four absent channels, bounded output and invalid-channel precedence\n", cases);
}
