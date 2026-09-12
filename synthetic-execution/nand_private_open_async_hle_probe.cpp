#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free compile coverage for PAL NANDPrivateOpenAsync
// (0x8019C990). Zero pointers exercise the safe invalid-argument path if this
// probe is ever executed, while the build still proves that the exact native
// boundary resolves through InvokeDirectCpu.
extern "C" __attribute__((used)) void synthetic_nand_private_open_async_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    ctx->gpr[5] = 1u;
    ctx->gpr[6] = 0u;
    ctx->gpr[7] = 0u;
    InvokeDirectCpu<0x8019C990u>(ctx);
}

#endif
