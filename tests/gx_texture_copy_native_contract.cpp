#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
struct Rect {
    std::int32_t x, y, width, height;
};
struct State {
    Rect texCopySrc;
    bool texCopySrcRenderSpace;
    GXTexFmt texCopyFmt;
    u16 texCopyDstWidth, texCopyDstHeight;
    bool texCopyHalfScale;
    GXFBClamp copyClamp;
    std::array<std::uint8_t, 32> unrelated;
} g_gxState;
State Seed() {
    State s;
    std::memset(&s, 0, sizeof(s));
    s.texCopySrc = {-1, 2, 3, -4};
    s.texCopySrcRenderSpace = true;
    s.texCopyFmt = GX_TF_RGB5A3;
    s.texCopyDstWidth = 123u;
    s.texCopyDstHeight = 456u;
    s.texCopyHalfScale = true;
    s.copyClamp = GX_CLAMP_BOTTOM;
    s.unrelated.fill(0xa5u);
    return s;
}
} // namespace

#include "pinned-texture-copy.inc"

int main() {
    unsigned cases = 0;
    for (unsigned c = 0; c < 4u; ++c) {
        g_gxState = Seed();
        auto expected = g_gxState;
        expected.copyClamp = static_cast<GXFBClamp>(c);
        GXSetCopyClamp(static_cast<GXFBClamp>(c));
        assert(std::memcmp(&g_gxState, &expected, sizeof(expected)) == 0);
        ++cases;
    }
    for (unsigned component = 0; component < 4u; ++component)
        for (unsigned value = 0; value < 65536u; ++value) {
            const std::array<u16, 4> values{
                static_cast<u16>(component == 0u ? value : 65535u),
                static_cast<u16>(component == 1u ? value : 32768u),
                static_cast<u16>(component == 2u ? value : 128u),
                static_cast<u16>(component == 3u ? value : 1u)};
            g_gxState = Seed();
            auto expected = g_gxState;
            expected.texCopySrc = {values[0], values[1], values[2], values[3]};
            expected.texCopySrcRenderSpace = false;
            GXSetTexCopySrc(values[0], values[1], values[2], values[3]);
            assert(std::memcmp(&g_gxState, &expected, sizeof(expected)) == 0);
            ++cases;
        }
    for (unsigned component = 0; component < 2u; ++component)
        for (unsigned value = 0; value < 65536u; ++value)
            for (const bool mipmap : {false, true}) {
                const auto width = static_cast<u16>(component == 0u ? value : 128u);
                const auto height = static_cast<u16>(component == 1u ? value : 128u);
                g_gxState = Seed();
                auto expected = g_gxState;
                expected.texCopyDstWidth = width;
                expected.texCopyDstHeight = height;
                expected.texCopyFmt = GX_TF_RGB5A3;
                expected.texCopyHalfScale = mipmap;
                GXSetTexCopyDst(width, height, GX_TF_RGB5A3, mipmap);
                assert(std::memcmp(&g_gxState, &expected, sizeof(expected)) == 0);
                ++cases;
            }
    assert(cases == 524292u);
    std::printf("PASS: pinned native texture-copy state fixtures=%u\n", cases);
}
