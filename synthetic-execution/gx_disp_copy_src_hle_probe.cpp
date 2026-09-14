#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016F438u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL GXSetDispCopySrc.
// r3=0 matches the real hardware blocker. The remaining arguments use a
// representative EGG video setup shape only to exercise the full four-argument
// narrowing and FIFO encoding path; they are not claimed as hardware evidence.
extern "C" __attribute__((used)) void synthetic_gx_disp_copy_src_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    ctx->gpr[5] = 640u;
    ctx->gpr[6] = 456u;
    InvokeDirectCpu<0x8016F438u>(ctx);
}

#endif
