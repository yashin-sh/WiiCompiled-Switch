#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80170474u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_chan_mat_color_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 4u;
    cpu.gpr[4] = 0u;
    InvokeDirectCpu<0x80170474u>(&cpu);
    return true;
}

#endif
