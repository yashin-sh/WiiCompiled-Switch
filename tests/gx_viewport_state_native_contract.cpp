#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using f32 = float;
using s32 = std::int32_t;
using u32 = std::uint32_t;
namespace {
struct State {
    float zScale, zOffset;
    s32 scissorOffsetX, scissorOffsetY;
    std::array<std::uint8_t, 16> unrelated;
} g_gxState{};
struct Registers {
    std::uint16_t bpSent;
    std::array<std::uint8_t, 16> unrelated;
} registers{}, *__gx = &registers;
struct Write {
    unsigned width;
    std::uint32_t value;
};
std::array<Write, 6> writes{};
unsigned count = 0;
std::uint16_t oldFlag = 0;
void Emit(unsigned width, std::uint32_t value) {
    assert(count < 6 && __gx->bpSent == oldFlag);
    writes[count++] = {width, value};
}
} // namespace
#define GX_WRITE_U8(value) Emit(8, static_cast<std::uint8_t>(value))
#define GX_WRITE_U32(value) Emit(32, static_cast<std::uint32_t>(value))
#define GX_WRITE_F32(value) Emit(32, std::bit_cast<std::uint32_t>(value))
#include "pinned-z-scale-offset.inc"
#define GX_WRITE_RAS_REG(value) \
    do {                        \
        Emit(8, 0x61);          \
        Emit(32, value);        \
    } while (false)
#include "pinned-scissor-offset.inc"
int main() {
    unsigned cases = 0;
    for (unsigned flag : {0u, 1u, 0xffffu})
        for (float scale : {0.f, -0.f, 1.f, -1.f, 0.5f, 1e20f})
            for (float offset : {0.f, -0.f, 1.f, -1.f, 0.5f, 1e20f}) {
                std::memset(&g_gxState, 0xa5, sizeof(g_gxState));
                std::memset(&registers, 0xa5, sizeof(registers));
                registers.bpSent = oldFlag = static_cast<std::uint16_t>(flag);
                auto expectedState = g_gxState;
                expectedState.zScale = scale;
                expectedState.zOffset = offset;
                auto expectedRegisters = registers;
                expectedRegisters.bpSent = 0;
                count = 0;
                GXSetZScaleOffset(scale, offset);
                assert(count == 6);
                const std::array<Write, 6> expectedWrites{{{8, 0x10}, {32, 0x101c}, {32, std::bit_cast<std::uint32_t>(16777215.f * offset)}, {8, 0x10}, {32, 0x101f}, {32, std::bit_cast<std::uint32_t>(1.f + 16777215.f * scale)}}};
                for (unsigned i = 0; i < 6; ++i)
                    assert(writes[i].width == expectedWrites[i].width && writes[i].value == expectedWrites[i].value);
                assert(std::memcmp(&g_gxState, &expectedState, sizeof(g_gxState)) == 0);
                assert(std::memcmp(&registers, &expectedRegisters, sizeof(registers)) == 0);
                ++cases;
            }
    std::printf("PASS: pinned depth XF fixtures=%u\n", cases);
    unsigned offsetCases = 0;
    const auto checkOffset = [&](s32 x, s32 y, unsigned flag) {
        std::memset(&g_gxState, 0xa5, sizeof(g_gxState));
        std::memset(&registers, 0xa5, sizeof(registers));
        registers.bpSent = oldFlag = static_cast<std::uint16_t>(flag);
        auto expectedState = g_gxState;
        expectedState.scissorOffsetX = x;
        expectedState.scissorOffsetY = y;
        auto expectedRegisters = registers;
        expectedRegisters.bpSent = 0;
        count = 0;
        GXSetScissorBoxOffset(x, y);
        const auto expectedWord = 0x59000000u | (static_cast<u32>((std::int64_t(y) + 342) / 2) << 10) | static_cast<u32>((std::int64_t(x) + 342) / 2);
        assert(count == 2 && writes[0].width == 8 && writes[0].value == 0x61 && writes[1].width == 32 && writes[1].value == expectedWord);
        assert(std::memcmp(&g_gxState, &expectedState, sizeof(g_gxState)) == 0);
        assert(std::memcmp(&registers, &expectedRegisters, sizeof(registers)) == 0);
        if (!x && !y)
            assert(expectedWord == 0x5902acabu);
        ++offsetCases;
    };
    for (auto flag : {0u, 1u, 0xffffu}) {
        for (s32 value = -342; value <= 1705; ++value) {
            checkOffset(value, 0, flag);
            checkOffset(0, value, flag);
        }
        for (auto x : {-342, -341, -1, 0, 1, 1704, 1705})
            for (auto y : {-342, -341, -1, 0, 1, 1704, 1705})
                checkOffset(x, y, flag);
    }
    std::printf("PASS: pinned scissor-offset BP fixtures=%u\n", offsetCases);
}
