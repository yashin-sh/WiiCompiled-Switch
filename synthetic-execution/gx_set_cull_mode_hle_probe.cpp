#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016F3B8u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetCullMode boundary. Hardware captured r3 = 2.
extern "C" __attribute__((used)) bool synthetic_gx_set_cull_mode_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 2u;
    InvokeDirectCpu<0x8016F3B8u>(&cpu);
    return true;
}

#endif
