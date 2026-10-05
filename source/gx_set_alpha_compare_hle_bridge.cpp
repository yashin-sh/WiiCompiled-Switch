#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>
#include <cstdlib>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>

// Existing renderer-owned flag, also consumed by EnsureDefaultGxAlphaCompare.
extern bool g_alphaCompareValid;
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_alpha_compare(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t compare0 = cpu->gpr[3];
    const std::uint32_t reference0 = cpu->gpr[4];
    const std::uint32_t operation = cpu->gpr[5];
    const std::uint32_t compare1 = cpu->gpr[6];
    const std::uint32_t reference1 = cpu->gpr[7];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_ALPHA_COMPARE");

    // Validate full enum words before narrowing or changing native state.
    // References follow the pinned wrapper's u8 conversion for any u32 input.
    if (compare0 >= 8u || compare1 >= 8u || operation >= 4u) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_ALPHA_COMPARE_UNPROVEN_ARGS", 0x80172088u, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    g_alphaCompareValid = true;
    GXSetAlphaCompare(
        static_cast<GXCompare>(compare0),
        static_cast<std::uint8_t>(reference0),
        static_cast<GXAlphaOp>(operation),
        static_cast<GXCompare>(compare1),
        static_cast<std::uint8_t>(reference1));
#else
    (void)reference0;
    (void)reference1;
#endif
}

#endif
