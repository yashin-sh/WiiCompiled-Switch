#include "abi_bridge.h"
#include "switch_os_hle_traits.hpp"

#include <cstdint>

static_assert(KnownNativeCpuCall<0x801AAD5Cu>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OSGetTime. The pinned HLE
// reads only the host-backed Broadway time base and returns the stable high/low
// words in guest r3:r4; it does not require guest memory or controller state.
extern "C" __attribute__((used)) void synthetic_os_get_time_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    const std::uint32_t savedR4 = cpu->gpr[4];
    InvokeDirectCpu<0x801AAD5Cu>(cpu);
    cpu->gpr[3] = savedR3;
    cpu->gpr[4] = savedR4;
}
