#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80173430u>::kAvailable);

// Nintendo-data-free compile/link coverage for the four-u32 PPC ABI seam.
// Synthetic CI keeps guest GXData absent, so the bridge verifies dispatch
// wiring without fabricating renderer state.
extern "C" __attribute__((used)) void synthetic_gx_set_scissor_hle_probe(CpuContext* ctx) {
    if (!ctx || ctx->pc != 0xFFFFFFF7u) {
        return;
    }

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    ctx->gpr[5] = 640u;
    ctx->gpr[6] = 480u;
    InvokeDirectCpu<0x80173430u>(ctx);
}

#endif
