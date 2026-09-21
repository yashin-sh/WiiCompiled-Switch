#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016E654u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_flush_hle_probe() {
    CpuContext cpu{};
    InvokeDirectCpu<0x8016E654u>(&cpu);
    return true;
}

#endif
