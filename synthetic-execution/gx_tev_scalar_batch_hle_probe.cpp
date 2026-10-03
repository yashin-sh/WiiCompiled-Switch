#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80171B58u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171CE0u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171D60u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171D20u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171DB8u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80171FD0u>::kAvailable);

// Retain the actual dispatch/bridges without executing a fabricated game
// setup. Host contracts separately exercise argument forwarding and refusals.
extern "C" __attribute__((used)) void synthetic_gx_tev_scalar_batch_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x80171B58u>(cpu);
    InvokeDirectCpu<0x80171CE0u>(cpu);
    InvokeDirectCpu<0x80171D60u>(cpu);
    InvokeDirectCpu<0x80171D20u>(cpu);
    InvokeDirectCpu<0x80171DB8u>(cpu);
    InvokeDirectCpu<0x80171FD0u>(cpu);
}

#endif
