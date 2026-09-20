#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_vtx_attr_fmt(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t vtxFmt = cpu->gpr[3];
    const std::uint32_t rawAttr = cpu->gpr[4];
    const std::uint32_t compCnt = cpu->gpr[5];
    const std::uint32_t compType = cpu->gpr[6];
    const std::uint32_t frac = cpu->gpr[7];

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_VTX_ATTR_FMT");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const std::uint32_t attr =
        rawAttr == static_cast<std::uint32_t>(GX_VA_NBT)
            ? static_cast<std::uint32_t>(GX_VA_NRM)
            : rawAttr;

    if (vtxFmt < 8u && attr < 26u) {
        const auto oldFmt = g_hleGxState.vtxAttrFmt[vtxFmt][attr];

        g_hleGxState.vtxAttrFmt[vtxFmt][attr].cnt =
            static_cast<GXCompCnt>(compCnt);
        g_hleGxState.vtxAttrFmt[vtxFmt][attr].type =
            static_cast<GXCompType>(compType);
        g_hleGxState.vtxAttrFmt[vtxFmt][attr].frac =
            static_cast<u8>(frac);

        if (rawAttr == static_cast<std::uint32_t>(GX_VA_NBT)) {
            g_hleGxState.vtxAttrFmt[vtxFmt][GX_VA_NBT] = {};
        }

        const auto& newFmt = g_hleGxState.vtxAttrFmt[vtxFmt][attr];
        if (oldFmt.cnt != newFmt.cnt ||
            oldFmt.type != newFmt.type ||
            oldFmt.frac != newFmt.frac) {
            g_hleGxState.InvalidateVtxLayoutHash();
        }
    }

    if (vtxFmt >= static_cast<std::uint32_t>(GX_MAX_VTXFMT) ||
        rawAttr < static_cast<std::uint32_t>(GX_VA_POS) ||
        rawAttr >= static_cast<std::uint32_t>(GX_VA_MAX_ATTR)) {
        return;
    }

    GXSetVtxAttrFmt(
        static_cast<GXVtxFmt>(vtxFmt),
        static_cast<GXAttr>(rawAttr),
        static_cast<GXCompCnt>(compCnt),
        static_cast<GXCompType>(compType),
        static_cast<u8>(frac));
#else
    (void)vtxFmt;
    (void)rawAttr;
    (void)compCnt;
    (void)compType;
    (void)frac;
#endif
}

#endif
