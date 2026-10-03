#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016F3E0u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetCoPlanar boundary. Discovery hardware captured r3=0.
extern "C" __attribute__((used)) bool synthetic_gx_set_co_planar_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0u;
    InvokeDirectCpu<0x8016F3E0u>(&cpu);
    return true;
}

#endif
