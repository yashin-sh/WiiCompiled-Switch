#include "abi_bridge.h"
#include "switch_thread_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801A9C08u>::kAvailable);
static_assert(KnownNativeCpuCall<0x801A1F58u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL SelectThread. The real
// hardware blocker entered with forceSwitch=0 (r3=0) from OSResumeThread.
// OSLoadContext is now the hardware-proven direct dependency selected by that
// bridge and remains covered transitively by the retained real SelectThread TU.
extern "C" __attribute__((noinline, used)) void synthetic_select_thread_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    const std::uint32_t savedR3 = cpu->gpr[3];
    cpu->gpr[3] = 0u;
    InvokeDirectCpu<0x801A9C08u>(cpu);
    cpu->gpr[3] = savedR3;
}
