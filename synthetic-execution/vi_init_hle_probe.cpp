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

    // Retain compile/link coverage for the runtime-call poll seam. The real
    // register-isolation path becomes active only after a HostContext-backed
    // guest OSThread is current; public CI deliberately has no game thread.
    // Hardware validation therefore remains the acceptance gate for #188's
    // first-fiber regression while this probe prevents the seam from dropping.
    ctx->gpr[3] = 0x89abcdefu;
    ctx->gpr[4] = 0x13579bdfu;
    mkw_switch_hle_vi_poll_retrace(ctx);
}

#endif
