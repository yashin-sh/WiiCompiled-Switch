#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016D3A4u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_vtx_desc_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 9u;
    cpu.gpr[4] = 1u;
    InvokeDirectCpu<0x8016D3A4u>(&cpu);
    return true;
}

#endif
