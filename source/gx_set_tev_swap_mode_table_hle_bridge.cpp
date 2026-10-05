#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_tev_swap_mode_table(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t id = cpu->gpr[3];
    const std::uint32_t red = cpu->gpr[4];
    const std::uint32_t green = cpu->gpr[5];
    const std::uint32_t blue = cpu->gpr[6];
    const std::uint32_t alpha = cpu->gpr[7];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_SWAP_MODE_TABLE");

    // Bound the unchecked pinned channel enums to their legal SDK domain.
    // Aurora retains the shared tevKsel fields and emits both BP commands.
    if (id >= 4u || red >= 4u || green >= 4u || blue >= 4u || alpha >= 4u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_SWAP_MODE_TABLE_UNPROVEN_ARGS", 0x8017200Cu, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTevSwapModeTable(
        static_cast<GXTevSwapSel>(id),
        static_cast<GXTevColorChan>(red),
        static_cast<GXTevColorChan>(green),
        static_cast<GXTevColorChan>(blue),
        static_cast<GXTevColorChan>(alpha));
#endif
}

#endif
