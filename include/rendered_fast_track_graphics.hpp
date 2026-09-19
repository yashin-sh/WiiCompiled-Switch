#pragma once

#include <cstdint>

struct CpuContext;

struct MkwSwitchRendererDiagnostics {
    bool initialized = false;
    bool frame_active = false;
    bool fifo_work_seen = false;
    std::uint64_t fifo_write_calls = 0;
    std::uint64_t display_list_calls = 0;
    std::uint64_t gx_copy_disp_calls = 0;
    std::uint64_t present_successes = 0;
    std::uint64_t present_failures = 0;
};

extern "C" bool mkw_switch_renderer_initialize() noexcept;
extern "C" void mkw_switch_renderer_shutdown() noexcept;
extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept;
extern "C" MkwSwitchRendererDiagnostics mkw_switch_renderer_diagnostics_snapshot() noexcept;
