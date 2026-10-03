#include "gx_display_list_native.hpp"
#include "dolphin/gx/__gx.h"

namespace {
std::uint8_t previousSave = 0;
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
