#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_pixel_fmt(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t pixel_format = cpu->gpr[3];
    const std::uint32_t z_format = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_PIXEL_FMT");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetPixelFmt(
        static_cast<GXPixelFmt>(pixel_format),
        static_cast<GXZFmt16>(z_format));
#else
    (void)pixel_format;
    (void)z_format;
#endif
}

#endif
