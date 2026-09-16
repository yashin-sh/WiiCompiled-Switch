#pragma once

#include <cstdint>

struct CpuContext;

namespace mkw::switch_guest_fiber {

// Adopt the HostContext scheduler already initialized by runtime_bootstrap.
// This must run on that scheduler stack before translated guest execution.
bool initialize_from_current_host() noexcept;

bool available() noexcept;
bool create(std::uint32_t guest_thread,
            std::uint32_t entry_point,
            std::uint32_t entry_arg,
            std::uint32_t guest_stack_top,
            CpuContext* seed_cpu) noexcept;
bool has(std::uint32_t guest_thread) noexcept;
std::uint32_t current_thread() noexcept;
bool register_current(std::uint32_t guest_thread, CpuContext* cpu) noexcept;
void suspend(std::uint32_t guest_thread) noexcept;
void resume(std::uint32_t guest_thread) noexcept;
bool switch_to(std::uint32_t guest_thread, CpuContext* cpu) noexcept;

} // namespace mkw::switch_guest_fiber
