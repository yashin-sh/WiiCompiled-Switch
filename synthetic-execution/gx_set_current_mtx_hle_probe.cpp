#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80173214u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_current_mtx_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 7u;
    InvokeDirectCpu<0x80173214u>(&cpu);
    return cpu.gpr[3] == 7u;
}

#endif
