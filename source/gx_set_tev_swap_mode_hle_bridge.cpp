#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_swap_mode(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t stage = cpu->gpr[3];
    const std::uint32_t raster = cpu->gpr[4];
    const std::uint32_t texture = cpu->gpr[5];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_SWAP_MODE");

    // This bring-up boundary accepts the audited SDK domain; the pinned
    // wrapper does not validate every enum. Unknown values remain blockers.
    if (stage >= 16u || raster >= 4u || texture >= 4u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_SWAP_MODE_UNPROVEN_ARGS", 0x80171FD0u, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTevSwapMode(
        static_cast<GXTevStageID>(stage),
        static_cast<GXTevSwapSel>(raster),
        static_cast<GXTevSwapSel>(texture));
#endif
}

#endif
