#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8017310Cu>::kAvailable);

// Compile/link coverage only. The bridge requires a valid guest 3x4 matrix,
// so synthetic CI keeps this path unreachable while retaining the direct-call
// edge and helper symbol.
extern "C" __attribute__((used)) void synthetic_gx_load_pos_mtx_imm_hle_probe(
    CpuContext* ctx) {
    if (!ctx || ctx->pc != 0xFFFFFFF6u) {
        return;
    }

    InvokeDirectCpu<0x8017310Cu>(ctx);
}

#endif
