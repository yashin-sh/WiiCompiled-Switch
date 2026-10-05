#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_alpha_op(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t stage = cpu->gpr[3];
    const std::uint32_t op = cpu->gpr[4];
    const std::uint32_t bias = cpu->gpr[5];
    const std::uint32_t scale = cpu->gpr[6];
    const std::uint32_t clamp = cpu->gpr[7];
    const std::uint32_t outReg = cpu->gpr[8];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_ALPHA_OP");

    // This bring-up boundary accepts the audited SDK domain; the pinned
    // wrapper does not validate every enum. Unknown values remain blockers.
    if (stage >= 16u || (op > 1u && op != 14u && op != 15u) ||
        bias >= 3u || scale >= 4u || outReg >= 4u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_ALPHA_OP_UNPROVEN_ARGS", 0x80171DB8u, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTevAlphaOp(
        static_cast<GXTevStageID>(stage),
        static_cast<GXTevOp>(op),
        static_cast<GXTevBias>(bias),
        static_cast<GXTevScale>(scale),
        static_cast<GXBool>(clamp != 0u),
        static_cast<GXTevRegID>(outReg));
#else
    (void)clamp;
#endif
}

#endif
