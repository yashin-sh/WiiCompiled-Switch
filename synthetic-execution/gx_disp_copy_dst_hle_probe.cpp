#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016F4B8u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL GXSetDispCopyDst.
// r3 uses the real hardware-observed width shape. The durable blocker record
// does not currently capture r4, so the probe uses a representative non-zero
// height solely to exercise the full two-argument path.
extern "C" __attribute__((used)) void synthetic_gx_disp_copy_dst_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0x00000260u;
    ctx->gpr[4] = 0x000001C8u;
    InvokeDirectCpu<0x8016F4B8u>(ctx);
}

#endif
