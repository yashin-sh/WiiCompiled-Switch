#include "abi_bridge.h"
#include "switch_thread_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801AA58Cu>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OSResumeThread. The real
// hardware blocker observed r3=0x8042A680, the same OSThread created by the
// preceding OSCreateThread boundary. The probe keeps that direct target and
// call shape represented in CI without embedding Nintendo data.
extern "C" __attribute__((noinline, used)) void synthetic_os_resume_thread_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    cpu->gpr[3] = 0x8042A680u;
    InvokeDirectCpu<0x801AA58Cu>(cpu);
    cpu->gpr[3] = savedR3;
}
