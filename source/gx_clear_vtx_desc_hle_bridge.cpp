#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_clear_vtx_desc(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_CLEAR_VTX_DESC");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Pinned WiiCompiled clears all 26 tracked vertex descriptors, invalidates
    // the cached vertex-layout hash only if state changed, preserves array
    // base/stride state, then mirrors the SDK call into Aurora GX.
    bool changed = false;
    for (int i = 0; i < 26; ++i) {
        changed |= g_hleGxState.vtxDesc[i] != GX_NONE;
        g_hleGxState.vtxDesc[i] = GX_NONE;
    }
    if (changed) {
        g_hleGxState.InvalidateVtxLayoutHash();
    }
    GXClearVtxDesc();
#endif
}

#endif
