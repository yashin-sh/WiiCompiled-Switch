#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016E37Cu>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171180u>::kAvailable);
static_assert(KnownNativeCpuCall<0x801711FCu>::kAvailable);

// Retain the actual dispatch/bridges without executing a fabricated game
// setup. Host contracts separately exercise each call with synthetic memory.
extern "C" __attribute__((used)) void synthetic_gx_tex_coord_batch_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x8016E37Cu>(cpu);
    InvokeDirectCpu<0x80171180u>(cpu);
    InvokeDirectCpu<0x801711FCu>(cpu);
}

#endif
