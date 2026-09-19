#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x801B94A4u>::kAvailable);
static_assert(KnownNativeCpuCall<0x801B9294u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL VIInit/__VIInit. The probe
// only exercises the native boundary; the bridge itself seeds generic VI
// bookkeeping defaults and never depends on game assets or a renderer.
extern "C" __attribute__((used)) void synthetic_vi_init_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0x12345678u;
    InvokeDirectCpu<0x801B94A4u>(ctx);
    InvokeDirectCpu<0x801B9294u>(ctx);
    mkw_switch_hle_vi_poll_retrace(ctx);
}

#endif
