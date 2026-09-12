#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free compile coverage for PAL OSReport (0x801A25D0).
// Pinned WiiCompiled handles this entry point as a host-only logging override;
// it does not alter CpuContext or guest memory. The Switch fast-track therefore
// preserves guest-visible state while intentionally sinking host formatting.
extern "C" __attribute__((used))
void synthetic_os_report_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    const std::uint32_t savedR3 = ctx->gpr[3];
    ctx->gpr[3] = 0u; // synthetic null format pointer; no Nintendo string data.
    InvokeDirectCpu<0x801A25D0u>(ctx);
    ctx->gpr[3] = savedR3;
}

#endif
