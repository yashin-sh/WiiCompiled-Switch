#include "capture.hpp"
#include "dolphin/gx.h"
#include "gx/gx.hpp"
#include "gx/fifo.hpp"

#include <bit>

namespace {
std::uint64_t read(std::span<const std::uint8_t> data, unsigned offset, unsigned count) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < count; ++i)
        value = (value << 8) | data[offset + i];
    return value;
}
replay::CopyState copy_state(bool texture, bool clear) {
    const auto& s = aurora::gx::g_gxState;
    const auto& rect = texture ? s.texCopySrc : s.dispCopySrc;
    replay::CopyState result{};
    result[0] = clear;
    result[1] = rect.x;
    result[2] = rect.y;
    result[3] = rect.width;
    result[4] = rect.height;
    result[5] = texture ? s.texCopyDstWidth : s.dispCopyDstWidth;
    result[6] = texture ? s.texCopyDstHeight : s.dispCopyDstHeight;
    result[7] = texture ? s.texCopyFmt : GX_TF_RGBA8;
    result[8] = texture && s.texCopyHalfScale;
    result[9] = texture && s.texCopySrcRenderSpace;
    result[10] = s.viewportPolicy;
    result[11] = s.copyClamp;
    result[12] = s.dispCopyFrame2Field;
    result[13] = s.dispCopyGamma;
    result[14] = std::bit_cast<std::uint32_t>(s.dispCopyYScale);
    result[15] = s.copyFilterAa;
    result[16] = s.copyFilterVf;
    for (unsigned i = 0; i < 24; ++i)
        result[17 + i] = s.copyFilterSamplePattern[i / 2][i % 2];
    for (unsigned i = 0; i < 7; ++i)
        result[41 + i] = s.copyFilterVFilter[i];
    for (unsigned i = 0; i < 4; ++i)
        result[48 + i] = std::bit_cast<std::uint32_t>(s.clearColor[i]);
    result[52] = s.clearDepth;
    result[53] = s.pixelFmt;
    result[54] = s.zFmt;
    result[55] = s.depthUpdate;
    result[56] = s.colorUpdate;
    result[57] = s.alphaUpdate;
    result[58] = s.dstAlpha;
    return result;
}
} // namespace

extern "C" void mkw_replay_capture_copy(bool texture, void* destination, bool clear) noexcept {
    if (!replay::recording())
        return;
    replay::capture_copy(texture ? replay::Kind::CopyTex : replay::Kind::CopyDisp,
                         reinterpret_cast<std::uintptr_t>(destination), copy_state(texture, clear));
}
extern "C" void mkw_replay_capture_mapping(unsigned policy) noexcept {
    replay::capture_mapping(policy);
}
namespace replay {
void apply_direct(Kind kind, std::span<const std::uint8_t> payload) {
    auto& s = aurora::gx::g_gxState;
    if (kind == Kind::Init) {
        GXInit(nullptr, 0);
        aurora::gx::fifo::clear_buffer();
        return;
    }
    if (kind == Kind::Mapping) {
        AuroraSetViewportPolicy(static_cast<AuroraViewportPolicy>(read(payload, 0, 4)));
        return;
    }
    const auto word = [&](unsigned i) { return static_cast<std::uint32_t>(read(payload, 8 + i * 4, 4)); };
    const bool texture = kind == Kind::CopyTex;
    auto& rect = texture ? s.texCopySrc : s.dispCopySrc;
    rect = {static_cast<int>(word(1)), static_cast<int>(word(2)), static_cast<int>(word(3)), static_cast<int>(word(4))};
    if (texture) {
        s.texCopyDstWidth = word(5);
        s.texCopyDstHeight = word(6);
        s.texCopyFmt = static_cast<GXTexFmt>(word(7));
        s.texCopyHalfScale = word(8);
        s.texCopySrcRenderSpace = word(9);
    } else {
        s.dispCopyDstWidth = word(5);
        s.dispCopyDstHeight = word(6);
    }
    s.viewportPolicy = static_cast<AuroraViewportPolicy>(word(10));
    s.copyClamp = static_cast<GXFBClamp>(word(11));
    s.dispCopyFrame2Field = word(12);
    s.dispCopyGamma = static_cast<GXGamma>(word(13));
    s.dispCopyYScale = std::bit_cast<float>(word(14));
    s.copyFilterAa = word(15);
    s.copyFilterVf = word(16);
    for (unsigned i = 0; i < 24; ++i)
        s.copyFilterSamplePattern[i / 2][i % 2] = word(17 + i);
    for (unsigned i = 0; i < 7; ++i)
        s.copyFilterVFilter[i] = word(41 + i);
    for (unsigned i = 0; i < 4; ++i)
        s.clearColor[i] = std::bit_cast<float>(word(48 + i));
    s.clearDepth = word(52);
    s.pixelFmt = static_cast<GXPixelFmt>(word(53));
    s.zFmt = static_cast<GXZFmt16>(word(54));
    s.depthUpdate = word(55);
    s.colorUpdate = word(56);
    s.alphaUpdate = word(57);
    s.dstAlpha = word(58);
    if (texture)
        GXCopyTex(reinterpret_cast<void*>(read(payload, 0, 8)), word(0));
    else
        GXCopyDisp(nullptr, word(0));
}
} // namespace replay
