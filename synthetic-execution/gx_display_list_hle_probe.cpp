#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
static_assert(KnownNativeCpuCall<0x80172A30u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80172E00u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80172EB4u>::kAvailable);
// Link retention only: no recording runs with fabricated startup arguments.
extern "C" __attribute__((used)) void synthetic_gx_display_list_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x80172E00u>(cpu);
    InvokeDirectCpu<0x80172A30u>(cpu);
    InvokeDirectCpu<0x80172EB4u>(cpu);
}
#endif
