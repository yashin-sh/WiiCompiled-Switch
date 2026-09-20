#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "memory.h"
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_chan_mat_color(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t channel = cpu->gpr[3];
    const std::uint32_t colorPtr = cpu->gpr[4];

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_CHAN_MAT_COLOR");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    EnsureAuroraFrameActive();
    const GXColor color = DecodeGxColor(Memory::Read32(colorPtr));
    GXSetChanMatColor(static_cast<GXChannelID>(channel), color);
#else
    (void)channel;
    (void)colorPtr;
#endif
}

#endif
