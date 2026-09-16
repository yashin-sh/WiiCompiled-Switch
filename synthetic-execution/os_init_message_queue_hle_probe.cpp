#include "abi_bridge.h"
#include "switch_os_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801A72FCu>::kAvailable);
static_assert(KnownNativeCpuCall<0x801A7424u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL OS__InitMessageQueue.
// The real hardware blocker observed r3=0x804294F0; the synthetic probe keeps
// the call shape explicit without embedding or requiring any Nintendo data.
extern "C" __attribute__((used)) void synthetic_os_init_message_queue_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801A72FCu>(cpu);
}

// Compile/link the hardware-proven PAL OSReceiveMessage native boundary without
// executing scheduler semantics against synthetic queue state. The branch is
// intentionally unreachable for the normal synthetic probe context while still
// forcing the trait/helper call edge into the translation unit.
extern "C" __attribute__((used)) void synthetic_os_receive_message_hle_probe(CpuContext* cpu) {
    if (!cpu || cpu->pc != 0xFFFFFFFFu) {
        return;
    }

    InvokeDirectCpu<0x801A7424u>(cpu);
}
