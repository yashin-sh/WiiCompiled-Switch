#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_z_comp_loc(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t beforeTexture = cpu->gpr[3];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_Z_COMP_LOC");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // TARGET_PC GXBool is bool: preserve the pinned full-word nonzero test.
    GXSetZCompLoc(static_cast<GXBool>(beforeTexture));
#else
    (void)beforeTexture;
#endif
}

#endif
