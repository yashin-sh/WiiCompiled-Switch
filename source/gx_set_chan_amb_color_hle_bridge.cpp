#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "memory.h"
#include <dolphin/gx.h>
#include <cstdlib>

// Matches the pinned gx_internal.h declaration; avoid importing unrelated
// desktop GX state just to call the existing rendered frame helper.
void EnsureAuroraFrameActive();

namespace {
[[noreturn]] void AbortInvalidColor(CpuContext* cpu) noexcept {
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_CHAN_AMB_COLOR_INVALID_COLOR");
    mkw_switch_report_unsupported_translated_dispatch(
        "GX_SET_CHAN_AMB_COLOR_INVALID_COLOR", cpu->gpr[4], cpu);
    std::abort();
}
} // namespace
#endif

extern "C" void mkw_switch_hle_gx_set_chan_amb_color(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t channel = cpu->gpr[3];
    const std::uint32_t colorAddress = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_CHAN_AMB_COLOR");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Pinned gx_lighting.cpp activates the frame before reading the color.
    EnsureAuroraFrameActive();
    if (!Memory::IsInitialized() || colorAddress == 0u ||
        !Memory::Contains(colorAddress, sizeof(std::uint32_t))) {
        AbortInvalidColor(cpu);
    }
    std::uint32_t word = 0u;
    try {
        word = Memory::Read32(colorAddress);
    } catch (...) {
        AbortInvalidColor(cpu);
    }
    // Exact DecodeGxColor conversion. Aurora retains its combined-channel
    // expansion, channel validation, cached state and XF register writes.
    const GXColor color{
        static_cast<std::uint8_t>(word >> 24),
        static_cast<std::uint8_t>(word >> 16),
        static_cast<std::uint8_t>(word >> 8),
        static_cast<std::uint8_t>(word)};
    GXSetChanAmbColor(static_cast<GXChannelID>(channel), color);
#else
    (void)channel;
    (void)colorAddress;
#endif
}

#endif
