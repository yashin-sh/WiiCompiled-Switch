#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8017054Cu>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_num_chans_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 1u;
    InvokeDirectCpu<0x8017054Cu>(&cpu);
    return true;
}

#endif
