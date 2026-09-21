#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x801722A8u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetNumTevStages boundary. The real-Switch blocker captured r3 = 1.
extern "C" __attribute__((used)) bool synthetic_gx_set_num_tev_stages_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 1u;
    InvokeDirectCpu<0x801722A8u>(&cpu);
    return true;
}

#endif
