#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_alpha_in(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t stage = cpu->gpr[3];
    const std::uint32_t a = cpu->gpr[4];
    const std::uint32_t b = cpu->gpr[5];
    const std::uint32_t c = cpu->gpr[6];
    const std::uint32_t d = cpu->gpr[7];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_ALPHA_IN");

    // This bring-up boundary accepts the audited SDK domain; the pinned
    // wrapper does not validate every enum. Unknown values remain blockers.
    if (stage >= 16u || a >= 8u || b >= 8u || c >= 8u || d >= 8u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_ALPHA_IN_UNPROVEN_ARGS", 0x80171D20u, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTevAlphaIn(
        static_cast<GXTevStageID>(stage),
        static_cast<GXTevAlphaArg>(a),
        static_cast<GXTevAlphaArg>(b),
        static_cast<GXTevAlphaArg>(c),
        static_cast<GXTevAlphaArg>(d));
#endif
}

#endif
