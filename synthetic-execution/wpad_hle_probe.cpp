#include "abi_bridge.h"
#include "switch_input_hle_traits.hpp"

static_assert(KnownNativeCpuCall<0x801BF5C4u>::kAvailable);
static_assert(KnownNativeCpuCall<0x801C329Cu>::kAvailable);
static_assert(KnownNativeCpuCall<0x801BF64Cu>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-proven PAL
// WPADInit boundary. The pinned HLE only initializes host-side WPAD contract
// state and returns success, so this probe is safe without guest memory.
extern "C" __attribute__((used)) void synthetic_wpad_init_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801BF5C4u>(cpu);
}

// Nintendo-data-free coverage for the hardware-proven PAL
// WPADGetDpdSensitivity boundary. Its pinned default is pure host-side state
// with no guest-memory or controller-device dependency.
extern "C" __attribute__((used)) void synthetic_wpad_dpd_sensitivity_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801C329Cu>(cpu);
}

// Nintendo-data-free coverage for PAL WPADGetStatus. This compiles the exact
// no-argument contract-state getter without constructing any controller device.
extern "C" __attribute__((used)) void synthetic_wpad_get_status_hle_probe(CpuContext* cpu) {
    if (!cpu) {
        return;
    }

    InvokeDirectCpu<0x801BF64Cu>(cpu);
}
