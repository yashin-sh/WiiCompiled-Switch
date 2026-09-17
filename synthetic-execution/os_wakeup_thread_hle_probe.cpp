#include "abi_bridge.h"
#include "switch_os_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801AAAA4u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OSWakeupThread. The real
// hardware blocker observed a live wait queue at 0x804294A4, but this probe
// deliberately does not fabricate guest scheduler state or execute a switch.
extern "C" __attribute__((used)) void synthetic_os_wakeup_thread_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801AAAA4u>(cpu);
}
