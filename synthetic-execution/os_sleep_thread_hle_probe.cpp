#include "abi_bridge.h"
#include "switch_os_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801AA9B8u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OSSleepThread. The real
// hardware blocker observed the receive wait queue at 0x804294F8, but this
// probe does not execute scheduler semantics against fabricated guest state.
extern "C" __attribute__((used)) void synthetic_os_sleep_thread_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801AA9B8u>(cpu);
}
