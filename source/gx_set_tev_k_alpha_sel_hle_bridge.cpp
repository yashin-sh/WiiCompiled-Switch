#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif
#include <cstdint>
#include <cstdlib>
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_k_alpha_sel(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    const std::uint32_t stage = cpu->gpr[3];
    const std::uint32_t selector = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_K_ALPHA_SEL");
    // The pinned wrapper forwards selector unchanged. The audited SDK domain
    // deliberately excludes reserved holes and values requiring truncation.
    if (stage >= 16u || !(selector < 8u || (selector >= 16u && selector < 32u))) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_TEV_K_ALPHA_SEL_UNPROVEN_ARGS", 0x80171F80u, cpu);
        std::abort();
    }
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTevKAlphaSel(static_cast<GXTevStageID>(stage), static_cast<GXTevKAlphaSel>(selector));
#endif
}
#endif
