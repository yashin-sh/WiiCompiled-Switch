#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x801733B4u>::kAvailable);

// Nintendo-data-free compile/link coverage for the six-float PPC ABI seam.
// Synthetic CI does not initialize Aurora, so the bridge only decodes f1..f6.
extern "C" __attribute__((used)) void synthetic_gx_set_viewport_hle_probe(CpuContext* ctx) {
    if (!ctx || ctx->pc != 0xFFFFFFF8u) {
        return;
    }

    ctx->fpr[1].d = 0.0;
    ctx->fpr[2].d = 0.0;
    ctx->fpr[3].d = 640.0;
    ctx->fpr[4].d = 480.0;
    ctx->fpr[5].d = 0.0;
    ctx->fpr[6].d = 1.0;
    InvokeDirectCpu<0x801733B4u>(ctx);
}

#endif
