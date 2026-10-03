#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_k_color(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t id = cpu->gpr[3];
    const std::uint32_t colorAddress = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_K_COLOR");

    // Match the pinned wrapper's ID-before-memory ordering. Reject the raw
    // word before any narrowing, lookup or native effects.
    if (id >= 4u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_K_COLOR_UNPROVEN_ARGS", 0x80171ED4u, cpu);
        std::abort();
    }
    // Resolve the complete byte range through the Switch memory seam. Guest
    // address zero is valid if mapped; no alignment or host-base cast is used.
    const auto* bytes = Memory::GetPointer(colorAddress, 4u);
    if (!bytes) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_K_COLOR_UNREADABLE_COLOR", 0x80171ED4u, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const GXColor color{bytes[0], bytes[1], bytes[2], bytes[3]};
    GXSetTevKColor(static_cast<GXTevKColorID>(id), color);
#endif
}

#endif
