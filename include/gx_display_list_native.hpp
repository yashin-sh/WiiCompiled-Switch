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
struct MkwGxSuState {
    std::uint32_t s[8];
    std::uint32_t t[8];
    std::uint8_t updated_mask;
};
// Validate native references before any writes, then emit pending SU state to
// the current FIFO (live before Begin, recorded before End). Only updated
// coordinates are authoritative for guest mirroring.
extern "C" bool mkw_switch_gx_native_flush_su_state(std::uint32_t manual, MkwGxSuState& state);
MkwGxDisplayListCursor mkw_switch_gx_display_list_cursor();
void mkw_switch_gx_native_begin_display_list(void* list, std::uint32_t size, std::uint8_t save);
std::uint32_t mkw_switch_gx_native_end_display_list();
void mkw_switch_gx_record_scalar(std::uint32_t value, std::uint32_t size);
void mkw_switch_gx_record_burst(const void* data, std::uint32_t size);

// Bounded recording-only SDK sphere adapter; false leaves state untouched.
extern "C" bool mkw_switch_gx_native_draw_sphere(std::uint32_t major, std::uint32_t minor);
