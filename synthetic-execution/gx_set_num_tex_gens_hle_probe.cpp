#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016E5A4u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetNumTexGens boundary. The real-Switch blocker captured r3 = 0.
extern "C" __attribute__((used)) bool synthetic_gx_set_num_tex_gens_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0u;
    InvokeDirectCpu<0x8016E5A4u>(&cpu);
    return true;
}

#endif
