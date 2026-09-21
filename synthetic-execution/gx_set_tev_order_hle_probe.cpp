#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8017214Cu>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetTevOrder boundary. Hardware captured r3 = 0; r4-r6 were not present
// in the blocker diagnostic, so zeros here are synthetic fixture values only.
extern "C" __attribute__((used)) bool synthetic_gx_set_tev_order_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0u;
    cpu.gpr[4] = 0u;
    cpu.gpr[5] = 0u;
    cpu.gpr[6] = 0u;
    InvokeDirectCpu<0x8017214Cu>(&cpu);
    return true;
}

#endif
