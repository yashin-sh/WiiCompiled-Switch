#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016EAB0u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-proven PAL
// GXDrawDone boundary. Synthetic CI does not initialize Aurora; the common
// bridge therefore exercises only the pinned guest-visible done bookkeeping.
extern "C" __attribute__((used)) void synthetic_gx_draw_done_hle_probe(CpuContext* ctx) {
    if (!ctx || ctx->pc != 0xFFFFFFFDu) {
        return;
    }

    InvokeDirectCpu<0x8016EAB0u>(ctx);
}

#endif
