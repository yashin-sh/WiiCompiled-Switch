#pragma once

#include <cstddef>
#include <cstdint>

namespace mkw::heap_runtime_diag {

struct State {
    bool env_override = false;
    std::uintptr_t env_addr = 0;
    std::size_t env_size = 0;
    std::uint32_t set_heap_rc = 0;
    std::uintptr_t selected_heap_addr = 0;
    std::size_t selected_heap_size = 0;
    std::uint64_t total_before = 0;
    std::uint64_t used_before = 0;
    std::uint64_t total_after = 0;
    std::uint64_t used_after = 0;
};

const State& state() noexcept;

} // namespace mkw::heap_runtime_diag
