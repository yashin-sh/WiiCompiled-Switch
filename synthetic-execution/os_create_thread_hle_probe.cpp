#include "abi_bridge.h"
#include "switch_thread_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801A9E84u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OSCreateThread. The guest
// thread pointer matches the real hardware-observed r3 shape; the remaining
// arguments are synthetic valid MEM1 values so no Nintendo data is required.
extern "C" __attribute__((noinline, used)) void synthetic_os_create_thread_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    const std::uint32_t savedR4 = cpu->gpr[4];
    const std::uint32_t savedR5 = cpu->gpr[5];
    const std::uint32_t savedR6 = cpu->gpr[6];
    const std::uint32_t savedR7 = cpu->gpr[7];
    const std::uint32_t savedR8 = cpu->gpr[8];
    const std::uint32_t savedR9 = cpu->gpr[9];

    cpu->gpr[3] = 0x8042A680u;
    cpu->gpr[4] = 0x80001000u;
    cpu->gpr[5] = 0x12345678u;
    cpu->gpr[6] = 0x8042C000u;
    cpu->gpr[7] = 0x00001000u;
    cpu->gpr[8] = 16u;
    cpu->gpr[9] = 0u;
    InvokeDirectCpu<0x801A9E84u>(cpu);

    cpu->gpr[3] = savedR3;
    cpu->gpr[4] = savedR4;
    cpu->gpr[5] = savedR5;
    cpu->gpr[6] = savedR6;
    cpu->gpr[7] = savedR7;
    cpu->gpr[8] = savedR8;
    cpu->gpr[9] = savedR9;
}
