#include "abi_bridge.h"
#include "guest_flat_memory.h"
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

static_assert(KnownNativeCpuCall<0x8019812Cu>::kAvailable);
namespace {
constexpr std::uint32_t base = 0x70000000u;
std::array<std::uint8_t, 1024> backing;
bool active = false;
unsigned cases = 0;
unsigned stages = 0;
unsigned reportOpens = 0;
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
void SetState(bool initialized) {
    g_wpad_initialized = initialized;
    g_wpad_dpd_sensitivity = 7;
    g_wpad_sync_device_callback = 0x12345678u;
    g_pad_initialized = !initialized;
}
CpuContext Context(std::uint32_t channel, std::uint32_t output, std::uint32_t count) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x39u + 23u * i);
    cpu.gpr[3] = channel;
    cpu.gpr[4] = output;
    cpu.gpr[5] = count;
    return cpu;
}
void Invoke(std::uint32_t channel, std::uint32_t output, std::uint32_t count, bool initialized) {
    SetState(initialized);
    backing.fill(0xa5);
    auto cpu = Context(channel, output, count);
    CpuContext expected;
    std::memcpy(&expected, &cpu, sizeof(cpu));
    expected.gpr[3] = 0;
    const auto previousStages = stages;
    const auto previousOpens = reportOpens;
    KnownNativeCpuCall<0x8019812Cu>::Invoke(&cpu);
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(stages == previousStages + 1);
    CheckState(initialized);
    const bool writes = channel < 4 && output && count;
    if (!writes)
        assert(reportOpens == previousOpens);
    // Independent SDK wire expectations: 0x38-byte entries, error at 0x29,
    // core/ACC_DPD format 2 at 0x36; all remaining bytes including padding zero.
    const std::size_t length = (count > 16 ? 16 : count) * 56;
    for (std::size_t i = 0; i < backing.size(); ++i) {
        std::uint8_t wanted = 0xa5;
        if (writes && base + i >= output && base + i < output + length) {
            const auto offset = (base + i - output) % 56;
            wanted = offset == 41 ? 0xff : offset == 54 ? 2
                                                        : 0;
        }
        assert(backing[i] == wanted);
    }
    ++cases;
}
void Refuse(std::uint32_t output, std::uint32_t count) {
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        SetState(false);
        backing.fill(0xa5);
        auto cpu = Context(2, output, count);
        std::memcpy(&refusedExpected, &cpu, sizeof(cpu));
        refusedCpu = &cpu;
        reported = false;
        KnownNativeCpuCall<0x8019812Cu>::Invoke(&cpu);
        _exit(1);
    }
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 77);
    ++cases;
}
} // namespace

// Allocation and diagnostics only: the production bridge and Memory slice run.
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
    if (std::strcmp(stage, "RMCP01_KPAD_UNIFIED_STATUS") == 0)
        ++stages;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target,
                                                                  CpuContext* cpu) noexcept {
    assert(std::strcmp(reason, "KPAD_UNIFIED_STATUS_RANGE") == 0);
    assert(target == 0x8019812Cu && cpu == refusedCpu);
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
extern "C" FILE* __real_fopen(const char*, const char*);
extern "C" FILE* __wrap_fopen(const char* path, const char* mode) {
    assert(std::strcmp(path, "sdmc:/switch/WiiCompiled-Switch/fast-track-kpad-unified-status.txt") == 0);
    assert(std::strcmp(mode, "w") == 0);
    ++reportOpens;
    return __real_fopen(path, mode);
}
int main() {
    const auto previousStages = stages;
    KnownNativeCpuCall<0x8019812Cu>::Invoke(nullptr);
    assert(stages == previousStages && reportOpens == 0);
    for (bool initialized : {false, true}) {
        for (auto channel : {0u, 1u, 2u, 3u, 4u, 0xffffffffu}) {
            Invoke(channel, 0, 16, initialized);
            Invoke(channel, base, 0, initialized);
        }
        Invoke(4, base, 1, initialized);
    }
    Refuse(base, 1);
    Memory::Config config;
    config.regions = {{"KPAD unified test", base, backing.size()}};
    Memory::Init(config);
    for (bool initialized : {false, true}) {
        for (unsigned channel = 0; channel < 4; ++channel) {
            for (unsigned offset = 0; offset <= 968; ++offset)
                Invoke(channel, base + offset, 1, initialized);
            for (unsigned count = 1; count <= 17; ++count)
                Invoke(channel, base + 1, count, initialized);
            for (unsigned offset = 0; offset <= 128; ++offset)
                Invoke(channel, base + offset, 0xffffffffu, initialized);
        }
        for (auto channel : {4u, 0xffffffffu})
            for (auto output : {0u, base + 1, base + 1023, 0xffffffffu})
                Invoke(channel, output, 1, initialized);
        Invoke(0, 0xffffffffu, 0, initialized);
    }
    for (auto output : {base - 1, base + 969, base + 1023, base + 1024, 0xffffffe0u, 0xffffffffu})
        Refuse(output, 1);
    Refuse(base + 129, 16);          // Complete first entry, partial last: no writes allowed.
    Refuse(base + 129, 0xffffffffu); // Clamp before multiplication/validation.
    const auto previousOpens = reportOpens;
    for (bool initialized : {false, false, true})
        for (unsigned channel = 0; channel < 4; ++channel)
            Invoke(channel, base + 1 + 56 * channel, 1, initialized);
    assert(reportOpens == previousOpens + 4);
    Memory::Reset();
    Invoke(0, 0, 1, false);
    Refuse(base + 1, 1);
    std::printf("PASS: KPAD unified status %u CPU/memory/state cases; full absent samples, bounded count and transition-only SD reports\n", cases);
}
