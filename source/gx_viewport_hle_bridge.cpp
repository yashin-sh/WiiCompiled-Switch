#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_viewport(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    // PPC EABI carries scalar float arguments in f1..f13. Pinned WiiCompiled's
    // GXSetViewport native override declares six float parameters, therefore
    // this exact PAL boundary consumes f1..f6 and leaves guest registers intact.
    const float left = static_cast<float>(cpu->fpr[1].d);
    const float top = static_cast<float>(cpu->fpr[2].d);
    const float width = static_cast<float>(cpu->fpr[3].d);
    const float height = static_cast<float>(cpu->fpr[4].d);
    const float nearZ = static_cast<float>(cpu->fpr[5].d);
    const float farZ = static_cast<float>(cpu->fpr[6].d);

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_VIEWPORT");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetViewport(left, top, width, height, nearZ, farZ);
#else
    (void)left;
    (void)top;
    (void)width;
    (void)height;
    (void)nearZ;
    (void)farZ;
#endif
}

#endif
