#include <dolphin/gx.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace {
// Only mode-word storage is replaced; execute the verbatim pinned function,
// register macro and filter table, with no hand-written native implementation.
struct GXTexObj_ {
    u32 mode0;
    u32 mode1;
};
} // namespace
#include "pinned-gx-depth-lod.inc"

int main() {
    GXTexObj_ state{0x95u, 0u};
    GXInitTexObjLOD(reinterpret_cast<GXTexObj*>(&state), GX_NEAR, GX_NEAR,
                    0.f, 0.f, 0.f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    assert(state.mode0 == 0x105u && state.mode1 == 0u);
    for (std::uint32_t seed = 0; seed < 1024u; ++seed) {
        const auto before0 = seed * 0x9e3779b9u;
        const auto before1 = seed * 0x85ebca6bu;
        state = {before0, before1};
        GXInitTexObjLOD(reinterpret_cast<GXTexObj*>(&state), GX_NEAR, GX_NEAR,
                        0.f, 0.f, 0.f, GX_FALSE, GX_FALSE, GX_ANISO_1);
        assert(state.mode0 == ((before0 & ~0x003ffff0u) | 0x100u));
        assert(state.mode1 == (before1 & 0xffff0000u));
    }
    std::puts("PASS: pinned native depth LOD exact 0x95 -> 0x105 fixture and 1024 preservation cases");
}
