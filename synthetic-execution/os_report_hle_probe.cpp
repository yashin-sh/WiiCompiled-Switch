#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free compile coverage for PAL OSReport (0x801A25D0).
// Pinned WiiCompiled handles this entry point as a host-only logging override;
// it does not alter CpuContext or guest memory. The Switch fast-track therefore
// preserves guest-visible state while intentionally sinking host formatting.
extern "C" __attribute__((used)) void synthetic_os_report_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    const std::uint32_t savedR3 = ctx->gpr[3];
    ctx->gpr[3] = 0u; // synthetic null format pointer; no Nintendo string data.
    InvokeDirectCpu<0x801A25D0u>(ctx);
    ctx->gpr[3] = savedR3;
}

// Compile the PAL OSGetConsoleType boundary through the exact static native
// dispatch seam. Runtime semantics are sourced from guest memory at 0x80003118,
// so this probe intentionally does not fabricate Nintendo/game data.
extern "C" __attribute__((used)) void synthetic_os_get_console_type_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }
    InvokeDirectCpu<0x8019F33Cu>(ctx);
}

// Compile the PAL OSGetResetCode boundary through the same native dispatch
// seam. Pinned WiiCompiled deliberately returns Cold Boot (0), avoiding Wii
// reset MMIO on the host; use a nonzero sentinel so the result is observable.
extern "C" __attribute__((used)) void synthetic_os_get_reset_code_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }
    ctx->gpr[3] = 0xA5A5A5A5u;
    InvokeDirectCpu<0x801A8A50u>(ctx);
}

// Nintendo-data-free compile coverage for PAL DCZeroRange (0x801A16E4).
// Exercise both a normal cached-MEM1 request and the exact unmapped top-of-
// address-space shape seen on real hardware. The Switch Memory slice returns
// nullptr for the latter instead of throwing upstream's AccessViolation, so
// DCZeroRange must return before calling memset.
extern "C" __attribute__((used)) void synthetic_dc_zero_range_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] = 0x80001000u;
    ctx->gpr[4] = 32u;
    InvokeDirectCpu<0x801A16E4u>(ctx);

    ctx->gpr[3] = 0xFFFFFFFFu;
    ctx->gpr[4] = 1u;
    InvokeDirectCpu<0x801A16E4u>(ctx);
}

// Nintendo-data-free compile coverage for PAL IPCCltInit (0x80193478).
// The native boundary internally dispatches translated IPCInit (0x80192F7C),
// reserves 4 KiB in the r13-relative IPC arena, and returns success. The probe
// exists to keep that mixed native->translated seam linkable in CI.
extern "C" __attribute__((used)) void synthetic_ipc_clt_init_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }
    InvokeDirectCpu<0x80193478u>(ctx);
}

#endif
