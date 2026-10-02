#include "abi_bridge.h"
#include "memory.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80171814u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171968u>::kAvailable);

// Dispatch/link coverage only. Synthetic data does not stand in for the
// hardware-observed guest matrix, whose contents were not captured.
extern "C" __attribute__((used)) bool synthetic_gx_indirect_batch_hle_probe() {
    constexpr std::uint32_t address = 0x70001000u;
    Memory::Config config;
    config.regions.push_back({"synthetic-indirect-matrix", address, 24u});
    Memory::Init(config);
    for (std::uint32_t i = 0; i < 6u; ++i) {
        Memory::Write32(address + 4u * i, i == 0u || i == 4u ? 0x3f000000u : 0u);
    }
    CpuContext cpu{};
    cpu.gpr[3] = 1u;
    cpu.gpr[4] = address;
    cpu.gpr[5] = 1u;
    InvokeDirectCpu<0x80171814u>(&cpu);
    const bool preserved = cpu.gpr[3] == 1u && cpu.gpr[4] == address && cpu.gpr[5] == 1u;
    cpu.gpr[3] = 0u;
    cpu.gpr[4] = 0u;
    cpu.gpr[5] = 0u;
    InvokeDirectCpu<0x80171968u>(&cpu);
    Memory::Reset();
    return preserved && cpu.gpr[3] == 0u && cpu.gpr[4] == 0u && cpu.gpr[5] == 0u;
}

#endif
