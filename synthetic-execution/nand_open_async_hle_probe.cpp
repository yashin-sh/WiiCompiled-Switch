#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free compile/link coverage for PAL NANDGetType / NANDOpen /
// NANDOpenAsync / NANDPrivateSafeOpenAsync
// (0x8019E770 / 0x8019C800 / 0x8019C918 / 0x8019D104). Null guest pointers
// deliberately exercise only safe invalid-argument paths while proving all
// four native dispatches link.
extern "C" __attribute__((used)) void synthetic_nand_open_async_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    InvokeDirectCpu<0x8019E770u>(ctx);

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    ctx->gpr[5] = 1u;
    InvokeDirectCpu<0x8019C800u>(ctx);

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    ctx->gpr[5] = 1u;
    ctx->gpr[6] = 0u;
    ctx->gpr[7] = 0u;
    InvokeDirectCpu<0x8019C918u>(ctx);

    ctx->gpr[3] = 0u;
    ctx->gpr[4] = 0u;
    ctx->gpr[5] = 1u;
    ctx->gpr[6] = 0u;
    ctx->gpr[7] = 0u;
    ctx->gpr[8] = 0u;
    ctx->gpr[9] = 0u;
    InvokeDirectCpu<0x8019D104u>(ctx);
}

#endif
