#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x801B99ECu>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL VIWaitForRetrace.
// Seed r3 with the real hardware-observed value; the pinned boundary ignores
// the incoming value and returns zero after one retrace wait/advance.
extern "C" __attribute__((used)) void synthetic_vi_wait_for_retrace_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 1u;
    InvokeDirectCpu<0x801B99ECu>(ctx);
}

#endif
