#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_blend_mode(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t type = cpu->gpr[3];
    const std::uint32_t src_factor = cpu->gpr[4];
    const std::uint32_t dst_factor = cpu->gpr[5];
    const std::uint32_t logic_op = cpu->gpr[6];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_BLEND_MODE");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetBlendMode(
        static_cast<GXBlendMode>(type),
        static_cast<GXBlendFactor>(src_factor),
        static_cast<GXBlendFactor>(dst_factor),
        static_cast<GXLogicOp>(logic_op));
#else
    (void)type;
    (void)src_factor;
    (void)dst_factor;
    (void)logic_op;
#endif
}

#endif
