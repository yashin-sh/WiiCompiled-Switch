#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_current_mtx(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t matrixId = cpu->gpr[3];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_CURRENT_MTX");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Pinned WiiCompiled forwards the PPC matrix id in r3 directly to Aurora.
    GXSetCurrentMtx(matrixId);
#else
    (void)matrixId;
#endif
}

#endif
