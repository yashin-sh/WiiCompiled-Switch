#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
struct State {
    std::uint32_t peCtrl;
    std::uint16_t bpSent;
    std::array<std::uint8_t, 16> unrelated;
} state{}, *__gx = &state;
struct Write {
    unsigned bits;
    std::uint32_t value;
};
std::array<Write, 2> writes{};
unsigned count = 0;
std::uint16_t oldFlag = 0;
void WriteU8(std::uint8_t value) {
    assert(count == 0u && __gx->bpSent == oldFlag);
    writes[count++] = {8u, value};
}
void WriteU32(std::uint32_t value) {
    assert(count == 1u && __gx->bpSent == oldFlag);
    writes[count++] = {32u, value};
}
} // namespace

// Pinned command macro and function body; only transport/storage are seams.
#define GX_WRITE_U8(value) WriteU8(static_cast<std::uint8_t>(value))
#define GX_WRITE_U32(value) WriteU32(static_cast<std::uint32_t>(value))
#include "pinned-pix-mode-sync.inc"

int main() {
    unsigned cases = 0;
    for (unsigned value = 0; value < 65536u; ++value) {
        std::memset(&state, 0xa5, sizeof(state));
        state.peCtrl = 0x43000000u | value;
        state.bpSent = static_cast<std::uint16_t>(value);
        oldFlag = state.bpSent;
        auto expected = state;
        expected.bpSent = 1u;
        count = 0;
        GXPixModeSync();
        assert(count == 2u && writes[0].bits == 8u && writes[0].value == 0x61u);
        assert(writes[1].bits == 32u && writes[1].value == expected.peCtrl);
        assert(std::memcmp(&state, &expected, sizeof(state)) == 0);
        ++cases;
    }
    std::printf("PASS: pinned PixModeSync BP fixtures=%u\n", cases);
}
