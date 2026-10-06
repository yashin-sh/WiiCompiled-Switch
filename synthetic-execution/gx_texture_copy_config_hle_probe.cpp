#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016F618u>::kAvailable);
static_assert(KnownNativeCpuCall<0x8016F478u>::kAvailable);
static_assert(KnownNativeCpuCall<0x8016F4DCu>::kAvailable);

// Retain all three bridges; startup never executes fabricated copy arguments.
extern "C" __attribute__((used)) void synthetic_gx_texture_copy_config_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x8016F618u>(cpu);
    InvokeDirectCpu<0x8016F478u>(cpu);
    InvokeDirectCpu<0x8016F4DCu>(cpu);
}

#endif
