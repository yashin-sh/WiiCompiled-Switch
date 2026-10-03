#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "gx_tex_coord_guest_mirror.hpp"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tex_coord_bias(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t coord = cpu->gpr[3];
    const std::uint32_t sEnable = cpu->gpr[4];
    const std::uint32_t tEnable = cpu->gpr[5];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEX_COORD_BIAS");
    if (coord >= 8u || sEnable > 1u || tEnable > 1u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEX_COORD_BIAS_UNPROVEN_ARGS", 0x801711FCu, cpu);
        std::abort();
    }
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTexCoordBias(
        static_cast<GXTexCoordID>(coord), static_cast<GXBool>(sEnable),
        static_cast<GXBool>(tEnable));
#endif
    mkw::switch_gx_tex_coord_mirror::Bias(coord, sEnable, tEnable);
}

#endif
