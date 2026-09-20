#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "memory.h"
#include <dolphin/gx.h>

namespace {

// Bit-exact local copy of the pinned WiiCompiled DecodeGxColor semantics.
// Keeping this tiny conversion here avoids linking the whole gx_utils.cpp
// object (and its unrelated GX state/dependencies) into the narrow rendered
// fast-track target.
GXColor DecodePinnedGxColor(std::uint32_t colorWord) noexcept {
    GXColor color{};
    color.r = static_cast<std::uint8_t>((colorWord >> 24) & 0xFFu);
    color.g = static_cast<std::uint8_t>((colorWord >> 16) & 0xFFu);
    color.b = static_cast<std::uint8_t>((colorWord >> 8) & 0xFFu);
    color.a = static_cast<std::uint8_t>(colorWord & 0xFFu);
    return color;
}

} // namespace
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
    const GXColor color = DecodePinnedGxColor(Memory::Read32(colorPtr));
    GXSetChanMatColor(static_cast<GXChannelID>(channel), color);
#else
    (void)channel;
    (void)colorPtr;
#endif
}

#endif
