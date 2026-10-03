#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80172088u>::kAvailable);

// Link retention only; synthetic startup does not claim console execution.
extern "C" __attribute__((used)) void synthetic_gx_alpha_compare_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x80172088u>(cpu);
}

#endif
