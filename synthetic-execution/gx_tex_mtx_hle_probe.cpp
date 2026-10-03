#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80173234u>::kAvailable);

// Retain direct dispatch and the real bridge. Host contracts separately
// supply mapped synthetic matrices; this link probe is not executed.
extern "C" __attribute__((used)) void synthetic_gx_tex_mtx_hle_probe(CpuContext* cpu) {
    InvokeDirectCpu<0x80173234u>(cpu);
}

#endif
