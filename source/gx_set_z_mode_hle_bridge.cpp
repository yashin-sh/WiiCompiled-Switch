#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_z_mode(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t compare_enable = cpu->gpr[3];
    const std::uint32_t compare_func = cpu->gpr[4];
    const std::uint32_t update_enable = cpu->gpr[5];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_Z_MODE");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetZMode(
        static_cast<GXBool>(compare_enable),
        static_cast<GXCompare>(compare_func),
        static_cast<GXBool>(update_enable));
#else
    (void)compare_enable;
    (void)compare_func;
    (void)update_enable;
#endif
}

#endif
