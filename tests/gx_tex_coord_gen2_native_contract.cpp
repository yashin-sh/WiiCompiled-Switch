#include <dolphin/gx.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
std::vector<u8> packet;
void Emit(unsigned bytes, u32 value) {
    for (unsigned i = 0; i < bytes; ++i)
        packet.push_back(value >> ((bytes - i - 1) * 8));
}
#define GX_WRITE_U8(value) Emit(1, static_cast<u8>(value))
#define GX_WRITE_U32(value) Emit(4, static_cast<u32>(value))
#define CHECK(condition, ...) assert(condition)
#include "pinned-gen2.inc"
} // namespace

int main() {
    unsigned cases = 0;
    for (u8 pattern : {u8(0), u8(0x55), u8(0xa5), u8(0xff)}) {
        for (unsigned coord = 0; coord < 8; ++coord) {
            for (u32 matrix : {30u, 60u}) {
                if (matrix == 30 && coord != 0)
                    continue;
                std::memset(&state, pattern, sizeof(state));
                const auto before = state;
                auto expectedState = before;
                const auto shift = coord < 4 ? (coord + 1) * 6 : (coord - 4) * 6;
                const auto previous = coord < 4 ? before.matIdxA : before.matIdxB;
                const auto shadow = (previous & ~(63u << shift)) | (matrix << shift);
                if (coord < 4)
                    expectedState.matIdxA = shadow;
                else
                    expectedState.matIdxB = shadow;
                expectedState.bpSent = 0;
                std::vector<u8> expected;
                const auto word = [&](u32 value) {
                    for (unsigned i = 0; i < 4; ++i)
                        expected.push_back(value >> (24 - 8 * i));
                };
                const auto xf = [&](u32 address, u32 value) {
                    expected.push_back(0x10);
                    word(address);
                    word(value);
                };
                xf(0x1040 + coord, 0x280); // TEX0 row, MTX2x4.
                xf(0x1050 + coord, 61);    // Identity post matrix, no normalize.
                expected.push_back(8);
                expected.push_back(coord < 4 ? 0x30 : 0x40);
                word(shadow);
                xf(coord < 4 ? 0x1018 : 0x1019, shadow);
                packet.clear();
                PinnedGXSetTexCoordGen2(static_cast<GXTexCoordID>(coord), GX_TG_MTX2x4,
                                        GX_TG_TEX0, matrix, GX_FALSE, GX_PTIDENTITY);
                assert(packet == expected);
                assert(std::memcmp(&state, &expectedState, sizeof(state)) == 0);
                ++cases;
            }
        }
    }
    std::printf("PASS: %u pinned Gen2 XF/CP packets and complete native-state canaries\n", cases);
}
