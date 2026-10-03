#pragma once
#include <cstdint>

// Transport seam implemented inside the checked Aurora target. Keep its private
// register/FIFO headers out of the translated-code and public syntax closure.
struct MkwGxDisplayListCursor {
    void* buffer;
    std::uint32_t capacity;
    std::uint32_t written;
    bool active;
    std::uint8_t save_context;
};
MkwGxDisplayListCursor mkw_switch_gx_display_list_cursor();
void mkw_switch_gx_native_begin_display_list(void* list, std::uint32_t size, std::uint8_t save);
std::uint32_t mkw_switch_gx_native_end_display_list();
void mkw_switch_gx_record_scalar(std::uint32_t value, std::uint32_t size);
void mkw_switch_gx_record_burst(const void* data, std::uint32_t size);
