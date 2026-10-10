#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
// Only storage and command transport are replaced. The functions and register
// macro included below are copied verbatim from the pinned Aurora sources.
struct State {
    std::uint32_t peCtrl;
    std::uint16_t bpSent;
} state{};
State* __gx = &state;
std::vector<std::uint32_t> commands;
} // namespace

#define GX_WRITE_RAS_REG(value) commands.push_back(value)
#include "pinned-gx-fog-pixel.inc"
#undef GX_WRITE_RAS_REG

int main() {
    unsigned fogCases = 0;
    for (unsigned tuple = 0; tuple < 3u; ++tuple)
        for (std::size_t channel = 0; channel < 4u; ++channel)
            for (unsigned value = 0; value < 256u; ++value) {
                std::array<std::uint8_t, 4> rgba{0x12, 0x34, 0x56, 0x78};
                rgba[channel] = static_cast<std::uint8_t>(value);
                commands.clear();
                state.bpSent = 0;
                state.peCtrl = 0x43123456u;
                GXSetFog(GX_FOG_NONE, tuple == 1u ? 1.0f : 0.0f, tuple == 2u ? 0.0f : 1.0f,
                         tuple == 0u ? 0.1f : 0.0f, tuple == 0u ? 1.0f : 0.0f,
                         GXColor{rgba[0], rgba[1], rgba[2], rgba[3]});
                // Fixed audited register fixture for the console's finite tuple.
                // NONE still emits all five BP registers; alpha is not in FOGCLR.
                const std::array<std::uint32_t, 5> expected{
                    tuple != 0u ? 0xee000000u : 0xee03ce38u,
                    tuple != 0u ? 0xef40000fu : 0xef471c82u,
                    tuple != 0u ? 0xf0000001u : 0xf0000002u, 0xf1000000u,
                    0xf2000000u | (std::uint32_t(rgba[0]) << 16u) |
                        (std::uint32_t(rgba[1]) << 8u) | rgba[2]};
                assert(commands.size() == expected.size());
                for (std::size_t i = 0; i < expected.size(); ++i)
                    assert(commands[i] == expected[i]);
                assert(state.bpSent == 1u && state.peCtrl == 0x43123456u);
                ++fogCases;
            }
    for (const auto initial : {0u, 0x43123456u, 0x43abcdefu, 0xffffffffu})
        for (const bool before : {false, true}) {
            commands.clear();
            state.peCtrl = initial;
            state.bpSent = 0;
            GXSetZCompLoc(before);
            const auto expected = (initial & ~0x40u) | (before ? 0x40u : 0u);
            assert(commands.size() == 1u && commands[0] == expected);
            assert(state.peCtrl == expected && state.bpSent == 1u);
        }
    std::printf("PASS: pinned native Fog BP fixture cases=%u; ZComp preserves other PE bits\n", fogCases);
}
