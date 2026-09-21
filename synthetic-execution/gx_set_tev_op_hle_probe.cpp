#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80171C4Cu>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetTevOp boundary. Hardware captured r3 = 0; r4 was not present in the
// blocker diagnostic, so mode=0 here is only a synthetic fixture value.
extern "C" __attribute__((used)) bool synthetic_gx_set_tev_op_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0u;
    cpu.gpr[4] = 0u;
    InvokeDirectCpu<0x80171C4Cu>(&cpu);
    return true;
}

#endif
