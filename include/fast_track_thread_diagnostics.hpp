#pragma once

#include <cstdint>

struct CpuContext;

extern "C" void mkw_switch_note_guest_thread_event(
    const char* kind,
    CpuContext* cpu,
    std::uint32_t threadPtr,
    std::uint32_t entryPoint,
    std::uint32_t entryArg,
    std::int32_t requestedPriority) noexcept;
