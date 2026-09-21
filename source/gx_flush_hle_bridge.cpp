#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_flush(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_FLUSH");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXFlush();
#endif
}

#endif
