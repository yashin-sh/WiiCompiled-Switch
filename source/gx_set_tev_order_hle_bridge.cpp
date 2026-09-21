#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_order(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t stage = cpu->gpr[3];
    const std::uint32_t tex_coord = cpu->gpr[4];
    const std::uint32_t tex_map = cpu->gpr[5];
    const std::uint32_t channel = cpu->gpr[6];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_ORDER");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    if (stage >= GX_MAX_TEVSTAGE) {
        return;
    }
    GXSetTevOrder(
        static_cast<GXTevStageID>(stage),
        static_cast<GXTexCoordID>(tex_coord),
        static_cast<GXTexMapID>(tex_map),
        static_cast<GXChannelID>(channel));
#else
    (void)stage;
    (void)tex_coord;
    (void)tex_map;
    (void)channel;
#endif
}

#endif
