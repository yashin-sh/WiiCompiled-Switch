#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#include <cstdlib>
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "gx_stream_common.h"
#include "memory.h"
#include <array>
#include <bit>
#include <cmath>
namespace aurora::gx::fifo {
bool in_display_list();
}
#endif
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x80084d20u, cpu);
    std::abort();
}
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
std::uint32_t Word(const std::uint8_t* p) noexcept {
    return (std::uint32_t{p[0]} << 24) | (std::uint32_t{p[1]} << 16) |
           (std::uint32_t{p[2]} << 8) | p[3];
}
float Scalar(const std::uint8_t* p, CpuContext* cpu) noexcept {
    const float value = std::bit_cast<float>(Word(p));
    if (!std::isfinite(value))
        Refuse("LYT_DRAW_QUAD_NONFINITE_INPUT", cpu);
    return value;
}
bool Layout(unsigned count, bool colors) noexcept {
    for (unsigned attr = 0; attr < 26; ++attr) {
        const bool enabled = attr == GX_VA_POS || (colors && attr == GX_VA_CLR0) ||
                             (attr >= GX_VA_TEX0 && attr < GX_VA_TEX0 + count);
        if (g_hleGxState.vtxDesc[attr] != (enabled ? GX_DIRECT : GX_NONE))
            return false;
    }
    const auto matches = [](unsigned attr, GXCompCnt cnt, GXCompType type) {
        const auto& fmt = g_hleGxState.vtxAttrFmt[GX_VTXFMT0][attr];
        return fmt.cnt == cnt && fmt.type == type;
    };
    if (!matches(GX_VA_POS, GX_POS_XY, GX_F32) ||
        (colors && !matches(GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8)))
        return false;
    for (unsigned i = 0; i < count; ++i)
        if (!matches(GX_VA_TEX0 + i, GX_TEX_ST, GX_F32))
            return false;
    return true;
}
#endif
} // namespace
extern "C" void mkw_switch_hle_lyt_draw_quad(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_LYT_DRAW_QUAD");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto count = cpu->gpr[5];
    const bool colors = cpu->gpr[7] != 0;
    if (count > 8)
        Refuse("LYT_DRAW_QUAD_INVALID_TEXCOORD_COUNT", cpu);
    if (g_hleGxState.inBegin || g_hleGxState.fifoByteCount != 0 ||
        IsDisplayListActive() || aurora::gx::fifo::in_display_list())
        Refuse("LYT_DRAW_QUAD_ACTIVE_STREAM", cpu);
    if (!Layout(count, colors))
        Refuse("LYT_DRAW_QUAD_UNSUPPORTED_LAYOUT", cpu);
    const auto* position = Memory::GetPointer(cpu->gpr[3], 8);
    const auto* size = Memory::GetPointer(cpu->gpr[4], 8);
    const auto* coords = count ? Memory::GetPointer(cpu->gpr[6], count * 32u) : nullptr;
    const auto* rgba = colors ? Memory::GetPointer(cpu->gpr[7], 16) : nullptr;
    if (!position || !size || (count && !coords) || (colors && !rgba))
        Refuse("LYT_DRAW_QUAD_UNREADABLE_INPUT", cpu);
    const float x0 = Scalar(position, cpu), y0 = Scalar(position + 4, cpu);
    const float x1 = static_cast<float>(x0 + Scalar(size, cpu));
    const float y1 = static_cast<float>(y0 - Scalar(size + 4, cpu));
    if (!std::isfinite(x1) || !std::isfinite(y1))
        Refuse("LYT_DRAW_QUAD_NONFINITE_EXTENT", cpu);
    // Validate the complete input and construct the bounded borrowed packet
    // before changing any native GX state. All reads permit unaligned addresses.
    for (unsigned offset = 0; offset < count * 32u; offset += 4)
        (void)Scalar(coords + offset, cpu);
    std::array<std::uint8_t, 3u + 4u * (8u + 4u + 8u * 8u)> packet{};
    unsigned cursor = 0;
    const auto append = [&](std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            packet[cursor++] = value >> (24 - i * 8);
    };
    packet[cursor++] = 0x80 | GX_VTXFMT0;
    packet[cursor++] = 0;
    packet[cursor++] = 4;
    // Pinned nw4r corner order is top-left, top-right, bottom-right, bottom-left.
    for (const unsigned corner : {0u, 1u, 3u, 2u}) {
        append(std::bit_cast<std::uint32_t>((corner & 1) ? x1 : x0));
        append(std::bit_cast<std::uint32_t>((corner & 2) ? y1 : y0));
        if (colors) {
            const auto* p = rgba + corner * 4;
            const unsigned alpha = unsigned{p[3]} * (cpu->gpr[8] & 255u) / 255u;
            append((Word(p) & 0xffffff00u) | alpha);
        }
        for (unsigned i = 0; i < count; ++i) {
            const auto* uv = coords + i * 32 + corner * 8;
            append(Word(uv));
            append(Word(uv + 4));
        }
    }
    EnsureAuroraFrameActive();
    GxStream::PublishAuroraVtxState(
        GX_VTXFMT0, {/*includeNbt=*/false, /*fmtLoopFirst=*/0, /*fmtLoopLast=*/25});
    for (int attr = 0; attr < 26; ++attr)
        if (attr != GX_VA_NBT)
            GXSetSourceVtxDesc(static_cast<GXAttr>(attr), g_hleGxState.vtxDesc[attr]);
    GxStream::EnsureDefaultGxAlphaCompare();
    // Native GXCallDisplayList drains then decodes synchronously. Unlike the
    // desktop-only optimized helper, this path is available in the Switch link.
    GXCallDisplayList(packet.data(), cursor);
    GXMarkFrameWork();
#else
    Refuse("LYT_DRAW_QUAD_REQUIRES_RENDERER", cpu);
#endif
}
#endif
