#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "nand_check_contract.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
constexpr std::uint32_t base = 0x9015b000u, observedOutput = 0x9015b034u;
struct Allocation {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<Allocation> allocations;
bool active = false, failWrite = false;
unsigned writes = 0, cases = 0;
const char* stage = nullptr;

bool ExpectedRange(std::uint32_t output) {
    if (!active || output == 0)
        return false;
    for (const auto& a : allocations) {
        if (output >= a.base &&
            std::uint64_t(output) + 4u <= std::uint64_t(a.base) + a.bytes.size())
            return true;
    }
    return false;
}

void Check(std::uint32_t output, std::uint32_t size, std::uint32_t count, bool fail = false) {
    for (auto& a : allocations)
        for (std::size_t i = 0; i < a.bytes.size(); ++i)
            a.bytes[i] = static_cast<std::uint8_t>(0x81u + i);
    auto expected = allocations;
    const bool valid = ExpectedRange(output);
    unsigned oracleWrites = 0;
    const auto oracle = NandCheckContract::WriteHealthyResult(
        output, [&](std::uint32_t address, std::size_t length) {
            assert(address == output && length == 4u);
            return valid; }, [&](std::uint32_t address, std::uint32_t word) {
            ++oracleWrites;
            assert(address == output && word == 0u);
            if (fail)
                return false;
            for (auto& a : expected)
                if (address >= a.base && std::uint64_t(address) + 4u <=
                                            std::uint64_t(a.base) + a.bytes.size())
                    std::fill_n(a.bytes.begin() + address - a.base, 4, 0);
            return true; });
    assert(oracle == (valid && !fail ? 0 : -8));
    assert(oracleWrites == (valid ? 1u : 0u));

    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = size;
    cpu.gpr[4] = count;
    cpu.gpr[5] = output;
    auto expectedCpu = cpu;
    expectedCpu.gpr[3] = static_cast<std::uint32_t>(oracle);
    writes = 0;
    failWrite = fail;
    stage = nullptr;
    KnownNativeCpuCall<0x8019EAD0u>::Invoke(&cpu);
    failWrite = false;
    assert(std::memcmp(&cpu, &expectedCpu, sizeof(cpu)) == 0);
    assert(stage && std::strcmp(stage, "RMCP01_NAND_CHECK") == 0);
    assert(writes == oracleWrites);
    assert(allocations.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        assert(allocations[i].bytes == expected[i].bytes);
    ++cases;
}
} // namespace

// Only guest allocation is synthetic. All ranges, endian writes and access
// exceptions come from the actual production Memory slice.
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    for (const auto& request : requests) {
        assert(request.size <= 64u);
        allocations.push_back({request.base, std::vector<std::uint8_t>(request.size)});
    }
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    for (auto& a : allocations)
        if (address == a.base)
            return a.bytes.data();
    return nullptr;
}
void Shutdown() noexcept {
    active = false;
    allocations.clear();
}
} // namespace GuestFlat

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
extern "C" void __real__ZN6Memory7Write32Ejj(std::uint32_t, std::uint32_t);
extern "C" void __wrap__ZN6Memory7Write32Ejj(std::uint32_t address, std::uint32_t value) {
    ++writes;
    if (failWrite)
        throw Memory::AccessViolation(address, 4u, "injected write failure");
    __real__ZN6Memory7Write32Ejj(address, value);
}

int main() {
    static_assert(KnownNativeCpuCall<0x8019EAD0u>::kAvailable);
    Check(observedOutput, 184, 4);
    stage = nullptr;
    KnownNativeCpuCall<0x8019EAD0u>::Invoke(nullptr);
    assert(stage == nullptr);
    Memory::Config config;
    config.regions = {{"observed-output-shape", base, 64u},
                      {"top-of-guest-space", 0xfffffff0u, 16u},
                      {"mapped-null", 0u, 8u}};
    Memory::Init(config);
    assert(Memory::Contains(0, 4));
    // Exact observed arguments and output; arbitrary first two parameters are
    // deliberately ignored by the pinned virtual NAND implementation.
    constexpr std::array<std::array<std::uint32_t, 2>, 6> arguments = {{{184u, 4u}, {0u, 0u}, {0u, 0xffffffffu}, {0xffffffffu, 0u}, {0xffffffffu, 0xffffffffu}, {1u, 7u}}};
    for (const auto& args : arguments) {
        for (std::uint32_t offset = 0; offset < 65u; ++offset)
            Check(base + offset, args[0], args[1]);
        for (std::uint32_t offset = 0; offset < 16u; ++offset)
            Check(0xfffffff0u + offset, args[0], args[1]);
        for (std::uint32_t offset = 0; offset < 9u; ++offset)
            Check(offset, args[0], args[1]);
        for (const auto address : {base - 1u, 0x70000000u, 0x1015b034u})
            Check(address, args[0], args[1]);
    }
    Check(observedOutput, 184, 4, true);
    Check(0xfffffffcu, 184, 4, true);
    Memory::Reset();
    Check(observedOutput, 184, 4);
    std::printf("PASS: NANDCheck cases=%u; exact four-byte output, -8 failures, full CPU/canary preservation\n", cases);
}
