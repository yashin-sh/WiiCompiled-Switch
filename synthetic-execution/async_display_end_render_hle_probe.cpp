#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free coverage for the hardware-observed indirect native
// EGG::AsyncDisplay::endRender target. Synthetic execution intentionally
// stops before its private RMCP01 translated callees.
extern "C" __attribute__((used)) void synthetic_async_display_end_render_hle_probe(
    CpuContext* ctx) {
    if (!ctx || ctx->pc != 0xFFFFFFFBu) {
        return;
    }

    ctx->gpr[3] = 0x1000u;
    InvokeIndirectCpu(0x8020FF9Cu, ctx);
}

#endif
