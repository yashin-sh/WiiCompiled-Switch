#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_ind_tex_coord_scale(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t indStage = cpu->gpr[3];
    const std::uint32_t scaleS = cpu->gpr[4];
    const std::uint32_t scaleT = cpu->gpr[5];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_IND_TEX_COORD_SCALE");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetIndTexCoordScale(
        static_cast<GXIndTexStageID>(indStage),
        static_cast<GXIndTexScale>(scaleS),
        static_cast<GXIndTexScale>(scaleT));
#else
    (void)indStage;
    (void)scaleS;
    (void)scaleT;
#endif
}

#endif
