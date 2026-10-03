#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80171ED4u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171E10u>::kAvailable);
static_assert(KnownNativeCpuCall<0x8017200Cu>::kAvailable);

// Link retention only: never fabricate a game color pointer at runtime.
extern "C" __attribute__((used)) void synthetic_gx_tev_color_batch_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x80171ED4u>(cpu);
    InvokeDirectCpu<0x80171E10u>(cpu);
    InvokeDirectCpu<0x8017200Cu>(cpu);
}

#endif
