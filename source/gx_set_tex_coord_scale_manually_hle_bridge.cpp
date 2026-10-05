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

extern "C" void mkw_switch_hle_gx_set_tex_coord_scale_manually(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t coord = cpu->gpr[3];
    const std::uint32_t enable = cpu->gpr[4];
    const std::uint32_t sSize = cpu->gpr[5];
    const std::uint32_t tSize = cpu->gpr[6];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEX_COORD_SCALE_MANUALLY");
    // Aurora indexes eight coordinates without a CHECK. Keep this audited
    // family within legal enum/boolean values before native or guest effects.
    if (coord >= 8u || enable > 1u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEX_COORD_SCALE_MANUALLY_UNPROVEN_ARGS", 0x80171180u, cpu);
        std::abort();
    }
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTexCoordScaleManually(
        static_cast<GXTexCoordID>(coord), static_cast<GXBool>(enable),
        static_cast<u16>(sSize), static_cast<u16>(tSize));
#endif
    // Pinned WiiCompiled calls GX first, then publishes best-effort guest
    // bookkeeping. Headless retains that bookkeeping without native GX.
    mkw::switch_gx_tex_coord_mirror::Scale(coord, enable, sSize, tSize);
}

#endif
