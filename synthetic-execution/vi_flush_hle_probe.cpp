#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x801BA9A4u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL VIFlush.
// Seed r3 with the real hardware-observed zero input shape.
extern "C" __attribute__((used)) void synthetic_vi_flush_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0u;
    InvokeDirectCpu<0x801BA9A4u>(ctx);
}

#endif
