#pragma once
#include <cstdint>

// These consumers only inspect/drain the FIFO. Keeping the inline writer out
// of their closure leaves exactly one checked writer implementation in Aurora.
namespace aurora::gx::fifo {
void drain();
std::uint32_t get_buffer_size();
} // namespace aurora::gx::fifo
