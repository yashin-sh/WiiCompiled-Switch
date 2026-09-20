#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016DC68u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_vtx_attr_fmt_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0u;
    cpu.gpr[4] = 9u;
    cpu.gpr[5] = 1u;
    cpu.gpr[6] = 4u;
    cpu.gpr[7] = 0u;
    InvokeDirectCpu<0x8016DC68u>(&cpu);
    return true;
}

#endif
