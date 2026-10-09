#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#include <cstdlib>
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x801720c0u, cpu);
    std::abort();
}
} // namespace
extern "C" void mkw_switch_hle_gx_set_z_texture(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_Z_TEXTURE");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto op = cpu->gpr[3];
    const auto format = cpu->gpr[4];
    if (op > static_cast<std::uint32_t>(GX_ZT_REPLACE))
        Refuse("GX_SET_Z_TEXTURE_INVALID_OP", cpu);
    // Native GX maps every other full format word to the Z24X8 encoding.
    // Normalize before constructing an enum, preserving that default without
    // passing out-of-range C++ enum values through the native ABI.
    GXTexFmt nativeFormat = GX_TF_Z24X8;
    if (format == static_cast<std::uint32_t>(GX_TF_Z8))
        nativeFormat = GX_TF_Z8;
    else if (format == static_cast<std::uint32_t>(GX_TF_Z16))
        nativeFormat = GX_TF_Z16;
    GXSetZTexture(static_cast<GXZTexOp>(op), nativeFormat, cpu->gpr[5]);
#else
    Refuse("GX_SET_Z_TEXTURE_REQUIRES_RENDERER", cpu);
#endif
}
#endif
