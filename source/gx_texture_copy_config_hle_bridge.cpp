#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>
#include <cstdlib>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"

// The existing rendered slice does not link gx_utils.cpp. Keep the pinned
// shared type and symbol so future copy execution can consume these fields.
TexCopyState g_texCopyState;
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {
[[maybe_unused, noreturn]] void Refuse(const char* reason, std::uint32_t target, CpuContext* cpu) {
    mkw_switch_report_unsupported_translated_dispatch(reason, target, cpu);
    std::abort();
}
} // namespace

extern "C" void mkw_switch_hle_gx_set_copy_clamp(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_COPY_CLAMP");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const std::uint32_t clamp = cpu->gpr[3];
    // GXFBClamp is an unfixed enum with range 0..3. Guard the entire word
    // before casting: narrowing/masking first would admit unproven inputs.
    if (clamp > 3u) {
        Refuse("GX_COPY_CLAMP_UNPROVEN_ARGS", 0x8016F618u, cpu);
    }
    GXSetCopyClamp(static_cast<GXFBClamp>(clamp));
    // Preserve the pinned wrapper's best-effort guest mirror, including
    // native-before-memory ordering, null pointer handling and partial writes
    // when a later access fails. Unsigned address addition is intentional.
    try {
        const std::uint32_t gd = Memory::Read32(kGXDataPtrAddr);
        if (gd) {
            Memory::Write32(gd + 0x23Cu, (Memory::Read32(gd + 0x23Cu) & 0xFFFFFFFCu) | clamp);
            Memory::Write32(gd + 0x24Cu, (Memory::Read32(gd + 0x24Cu) & 0xFFFFFFFCu) | clamp);
        }
    } catch (...) {
    }
#else
    Refuse("GX_TEXTURE_COPY_REQUIRES_RENDERER", 0x8016F618u, cpu);
#endif
}

extern "C" void mkw_switch_hle_gx_set_tex_copy_src(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEX_COPY_SRC");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto left = static_cast<std::uint16_t>(cpu->gpr[3]);
    const auto top = static_cast<std::uint16_t>(cpu->gpr[4]);
    const auto width = static_cast<std::uint16_t>(cpu->gpr[5]);
    const auto height = static_cast<std::uint16_t>(cpu->gpr[6]);
    GXSetTexCopySrc(left, top, width, height);
    g_texCopyState.srcLeft = left;
    g_texCopyState.srcTop = top;
    g_texCopyState.srcWidth = width;
    g_texCopyState.srcHeight = height;
#else
    Refuse("GX_TEXTURE_COPY_REQUIRES_RENDERER", 0x8016F478u, cpu);
#endif
}

extern "C" void mkw_switch_hle_gx_set_tex_copy_dst(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEX_COPY_DST");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto width = static_cast<std::uint16_t>(cpu->gpr[3]);
    const auto height = static_cast<std::uint16_t>(cpu->gpr[4]);
    const auto format = cpu->gpr[5];
    const auto mipmap = cpu->gpr[6];
    // Only RGB5A3 is forecast by the checked Mii texture caller. Broader
    // formats stay blocked before enum conversion or any state mutation.
    if (format != 5u) {
        Refuse("GX_TEX_COPY_DST_UNPROVEN_FORMAT", 0x8016F4DCu, cpu);
    }
    GXSetTexCopyDst(width, height, static_cast<GXTexFmt>(format), static_cast<GXBool>(mipmap));
    g_texCopyState.dstWidth = width;
    g_texCopyState.dstHeight = height;
    g_texCopyState.dstFormat = format;
    // Aurora receives bool; WiiCompiled retains the complete raw word here.
    g_texCopyState.dstMipmap = mipmap;
#else
    Refuse("GX_TEXTURE_COPY_REQUIRES_RENDERER", 0x8016F4DCu, cpu);
#endif
}

#endif
