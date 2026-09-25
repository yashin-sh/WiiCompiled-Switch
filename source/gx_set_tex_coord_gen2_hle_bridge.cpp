#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>
#include <cstdlib>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {

constexpr std::uint32_t kGxSetTexCoordGen2Address = 0x8016E37Cu;
constexpr std::uint32_t kObservedDst = 0u;
constexpr std::uint32_t kObservedType = 1u;
constexpr std::uint32_t kObservedSrc = 4u;
constexpr std::uint32_t kObservedMtx = 60u;
constexpr std::uint32_t kObservedNormalize = 0u;
constexpr std::uint32_t kObservedPostMtx = 125u;

[[noreturn]] void AbortUnproven(CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(
        "GX_SET_TEX_COORD_GEN2_UNPROVEN_ARGS",
        kGxSetTexCoordGen2Address,
        cpu);
    std::abort();
}

} // namespace

extern "C" void mkw_switch_hle_gx_set_tex_coord_gen2(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t dst = cpu->gpr[3];
    const std::uint32_t type = cpu->gpr[4];
    const std::uint32_t src = cpu->gpr[5];
    const std::uint32_t mtx = cpu->gpr[6];
    const std::uint32_t normalize = cpu->gpr[7];
    const std::uint32_t postMtx = cpu->gpr[8];

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEX_COORD_GEN2");

    if (dst != kObservedDst ||
        type != kObservedType ||
        src != kObservedSrc ||
        mtx != kObservedMtx ||
        normalize != kObservedNormalize ||
        postMtx != kObservedPostMtx) {
        AbortUnproven(cpu);
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetTexCoordGen2(
        GX_TEXCOORD0,
        GX_TG_MTX2x4,
        GX_TG_TEX0,
        GX_IDENTITY,
        GX_FALSE,
        GX_PTIDENTITY);
#endif
}

#endif
