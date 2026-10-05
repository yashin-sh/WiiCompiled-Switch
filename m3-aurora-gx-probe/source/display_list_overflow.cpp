#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace aurora::gx::fifo::detail {
extern std::uint32_t sDlSize;
extern std::uint32_t sDlWritePos;
} // namespace aurora::gx::fifo::detail

// The checked build header calls this even with NDEBUG. Refuse before touching
// bytes outside the recording buffer; keep a durable diagnostic for MTP.
extern "C" [[noreturn]] void mkw_switch_gx_display_list_overflow(std::uint32_t length) {
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-gx-display-list-overflow.txt", "w")) {
        std::fprintf(out, "status=overflow-refused\ncapacity=%u\ncursor=%u\nrequested=%u\n",
                     aurora::gx::fifo::detail::sDlSize,
                     aurora::gx::fifo::detail::sDlWritePos, length);
        std::fclose(out);
    }
    std::abort();
}
