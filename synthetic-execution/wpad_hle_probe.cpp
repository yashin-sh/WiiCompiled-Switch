#include "abi_bridge.h"
#include "switch_input_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801BF5C4u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-proven PAL
// WPADInit boundary. The pinned HLE only initializes host-side WPAD contract
// state and returns success, so this probe is safe without guest memory.
extern "C" __attribute__((used)) void synthetic_wpad_init_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801BF5C4u>(cpu);
}
