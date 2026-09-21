#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_cull_mode(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t mode = cpu->gpr[3];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_CULL_MODE");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetCullMode(static_cast<GXCullMode>(mode));
#else
    (void)mode;
#endif
}

#endif
