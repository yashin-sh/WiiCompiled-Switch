#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80170F2Cu>::kAvailable);

// Retain the actual direct dispatch and bridge. Executable host contracts
// independently initialize guest descriptors and synthetic texture backing.
extern "C" __attribute__((used)) void synthetic_gx_texture_load_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x80170F2Cu>(cpu);
}

#endif
