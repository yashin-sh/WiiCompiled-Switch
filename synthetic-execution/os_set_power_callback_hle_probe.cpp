#include "abi_bridge.h"
#include "switch_os_power_hle_traits.hpp"

#include <cstdint>

static_assert(KnownNativeCpuCall<0x801AB75Cu>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OSSetPowerCallback. The
// production bridge touches only guest SDA bookkeeping and the shared interrupt
// state; no real /dev/stm or Nintendo data is required for this probe.
extern "C" __attribute__((used)) void synthetic_os_set_power_callback_hle_probe(
    CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    const std::uint32_t savedR13 = cpu->gpr[13];

    // An unmapped synthetic SDA exercises the compile/link path without
    // requiring a guest-memory fixture. Runtime semantic state is covered by
    // the same production trait that the local hardware fast-track dispatches.
    cpu->gpr[3] = 0u;
    cpu->gpr[13] = 0u;
    InvokeDirectCpu<0x801AB75Cu>(cpu);

    cpu->gpr[3] = savedR3;
    cpu->gpr[13] = savedR13;
}
