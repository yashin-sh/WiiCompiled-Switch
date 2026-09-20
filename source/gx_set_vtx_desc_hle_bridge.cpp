#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_vtx_desc(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t rawAttr = cpu->gpr[3];
    const std::uint32_t rawType = cpu->gpr[4];

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_VTX_DESC");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const std::uint32_t attr =
        rawAttr == static_cast<std::uint32_t>(GX_VA_NBT)
            ? static_cast<std::uint32_t>(GX_VA_NRM)
            : rawAttr;

    if (attr >= 26u || attr == static_cast<std::uint32_t>(GX_VA_NULL)) {
        return;
    }

    const GXAttrType oldType = g_hleGxState.vtxDesc[attr];
    g_hleGxState.vtxDesc[attr] = static_cast<GXAttrType>(rawType);

    if (rawAttr == static_cast<std::uint32_t>(GX_VA_NBT)) {
        g_hleGxState.vtxDesc[GX_VA_NBT] = GX_NONE;
    }

    if (g_hleGxState.vtxDesc[attr] != oldType) {
        g_hleGxState.InvalidateVtxLayoutHash();
    }

    if (IsMatrixIndexAttr(static_cast<GXAttr>(attr))) {
        return;
    }

    const GXAttrType auroraType =
        (rawType == static_cast<std::uint32_t>(GX_INDEX8) ||
         rawType == static_cast<std::uint32_t>(GX_INDEX16))
            ? GX_DIRECT
            : static_cast<GXAttrType>(rawType);

    GXSetVtxDesc(static_cast<GXAttr>(rawAttr), auroraType);
#else
    (void)rawAttr;
    (void)rawType;
#endif
}

#endif
