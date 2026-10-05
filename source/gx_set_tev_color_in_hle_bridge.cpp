#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_color_in(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t stage = cpu->gpr[3];
    const std::uint32_t a = cpu->gpr[4];
    const std::uint32_t b = cpu->gpr[5];
    const std::uint32_t c = cpu->gpr[6];
    const std::uint32_t d = cpu->gpr[7];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_COLOR_IN");

    // This bring-up boundary accepts the audited SDK domain; the pinned
    // wrapper does not validate every enum. Unknown values remain blockers.
    if (stage >= 16u || a >= 16u || b >= 16u || c >= 16u || d >= 16u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_COLOR_IN_UNPROVEN_ARGS", 0x80171CE0u, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTevColorIn(
        static_cast<GXTevStageID>(stage),
        static_cast<GXTevColorArg>(a),
        static_cast<GXTevColorArg>(b),
        static_cast<GXTevColorArg>(c),
        static_cast<GXTevColorArg>(d));
#endif
}

#endif
