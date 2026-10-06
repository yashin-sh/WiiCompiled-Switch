#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdlib>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x8016EB70u, cpu);
    std::abort();
}
} // namespace

extern "C" void mkw_switch_hle_gx_pix_mode_sync(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_PIX_MODE_SYNC");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Pinned gx_stubs.cpp: best-effort guest halfword update BEFORE the native
    // pixel-engine control BP command. Preserve unsigned address wraparound.
    try {
        const std::uint32_t gd = Memory::Read32(kGXDataPtrAddr);
        if (gd)
            Memory::Write16(gd + 2u, 0);
    } catch (...) {
    }
    try {
        GXPixModeSync();
    } catch (...) {
        Refuse("GX_PIX_MODE_SYNC_NATIVE_EXCEPTION", cpu);
    }
#else
    Refuse("GX_PIX_MODE_SYNC_REQUIRES_RENDERER", cpu);
#endif
}

#endif
