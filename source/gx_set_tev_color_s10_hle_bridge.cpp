#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#include <cstdlib>
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "memory.h"
#include <bit>
#include <dolphin/gx.h>
#endif
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x80171e70u, cpu);
    std::abort();
}
} // namespace
extern "C" void mkw_switch_hle_gx_set_tev_color_s10(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_COLOR_S10");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto id = cpu->gpr[3];
    if (id >= 4u)
        Refuse("GX_SET_TEV_COLOR_S10_INVALID_ID", cpu);
    const auto* bytes = Memory::GetPointer(cpu->gpr[4], 8u);
    if (!bytes)
        Refuse("GX_SET_TEV_COLOR_S10_UNREADABLE_COLOR", cpu);
    // Preserve every signed 16-bit encoding. Native GX masks to eleven bits;
    // pre-clamping the SDK range would change the pinned wire semantics.
    const auto component = [&](unsigned offset) {
        const auto word = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(bytes[offset]) << 8) | bytes[offset + 1]);
        return std::bit_cast<std::int16_t>(word);
    };
    const GXColorS10 color{component(0), component(2), component(4), component(6)};
    GXSetTevColorS10(static_cast<GXTevRegID>(id), color);
#else
    Refuse("GX_SET_TEV_COLOR_S10_REQUIRES_RENDERER", cpu);
#endif
}
#endif
