#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x801722CCu>::kAvailable);
static_assert(KnownNativeCpuCall<0x80172858u>::kAvailable);

// Link retention only; startup does not execute fabricated game arguments.
extern "C" __attribute__((used)) void synthetic_gx_fog_z_comp_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x801722CCu>(cpu);
    InvokeDirectCpu<0x80172858u>(cpu);
}

#endif
