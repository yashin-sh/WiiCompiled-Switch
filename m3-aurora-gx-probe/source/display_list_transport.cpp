#include "gx_display_list_native.hpp"
#include "dolphin/gx/__gx.h"

namespace {
std::uint8_t previousSave = 0;
}
bool mkw_switch_gx_native_flush_su_state(std::uint32_t manual, MkwGxSuState& state) {
    if (manual > 0xFFu || manual != __gx->tcsManEnab) {
        return false;
    }
    std::uint8_t updated = 0;
    const auto select = [&](std::uint32_t map, std::uint32_t coord) {
        if (manual & (1u << coord)) {
            return true;
        }
        if (map >= 8u) {
            return false;
        }
        updated |= static_cast<std::uint8_t>(1u << coord);
        return true;
    };
    const auto indirect = static_cast<unsigned>(GET_REG_FIELD(__gx->genMode, 3, 16));
    // The pinned SU loop has four indirect reference layouts; reject impossible
    // native state instead of letting its default layout stand in for stages 4+.
    if (indirect > 4u) {
        return false;
    }
    for (unsigned i = 0; i < indirect; ++i) {
        if (!select((__gx->iref >> (6u * i)) & 7u, (__gx->iref >> (6u * i + 3u)) & 7u)) {
            return false;
        }
    }
    const auto stages = static_cast<unsigned>(GET_REG_FIELD(__gx->genMode, 4, 10)) + 1u;
    for (unsigned i = 0; i < stages; ++i) {
        const auto map = __gx->texmapId[i] & ~0x100u;
        const auto coord = (__gx->tref[i / 2u] >> ((i & 1u) ? 15u : 3u)) & 7u;
        if (map != 0xFFu && (__gx->texmapValid & (1u << i)) && !select(map, coord)) {
            return false;
        }
    }
    // Guest dirty bit 0 may outlive a preceding native flush. Re-emit through
    // the actual pinned routine; clearing the guest marker alone is insufficient.
    __GXSetSUTexRegs();
    __gx->dirtyState &= ~1u;
    for (unsigned i = 0; i < 8; ++i) {
        state.s[i] = __gx->suTs0[i];
        state.t[i] = __gx->suTs1[i];
    }
    state.updated_mask = updated;
    return true;
}
MkwGxDisplayListCursor mkw_switch_gx_display_list_cursor() {
    using namespace aurora::gx::fifo::detail;
    return {sDlBuffer, sDlSize, sDlWritePos, sInDisplayList, __gx->dlSaveContext};
}
void mkw_switch_gx_native_begin_display_list(void* list, std::uint32_t size, std::uint8_t save) {
    previousSave = __gx->dlSaveContext;
    __gx->dlSaveContext = save;
    GXBeginDisplayList(list, size);
}
std::uint32_t mkw_switch_gx_native_end_display_list() {
    const auto bytes = GXEndDisplayList();
    __gx->dlSaveContext = previousSave;
    return bytes;
}
void mkw_switch_gx_record_scalar(std::uint32_t value, std::uint32_t size) {
    switch (size) {
    case 1:
        aurora::gx::fifo::write_u8(static_cast<std::uint8_t>(value));
        break;
    case 2:
        aurora::gx::fifo::write_u16(static_cast<std::uint16_t>(value));
        break;
    case 4:
        aurora::gx::fifo::write_u32(value);
        break;
    }
}
void mkw_switch_gx_record_burst(const void* data, std::uint32_t size) {
    aurora::gx::fifo::write_data(data, size);
}
