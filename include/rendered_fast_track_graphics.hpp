#pragma once

#include <cstdint>

struct CpuContext;

struct MkwSwitchRendererDiagnostics {
    bool initialized = false;
    bool frame_active = false;
    bool fifo_work_seen = false;
    std::uint64_t fifo_write_calls = 0;
    std::uint64_t fifo_write8_calls = 0;
    std::uint64_t fifo_write16_calls = 0;
    std::uint64_t fifo_write32_calls = 0;
    std::uint64_t fifo_write_float_calls = 0;
    std::uint64_t bp_reg_49_calls = 0;
    std::uint64_t bp_reg_4a_calls = 0;
    std::uint64_t bp_reg_4d_calls = 0;
    std::uint64_t bp_reg_other_calls = 0;
    std::uint32_t last_fifo_value = 0;
    std::uint32_t last_bp_word = 0;
    std::uint8_t last_fifo_size = 0;
    std::uint64_t display_list_calls = 0;
    std::uint64_t gx_copy_disp_calls = 0;
    std::uint64_t present_successes = 0;
    std::uint64_t present_failures = 0;
};

extern "C" bool mkw_switch_renderer_initialize() noexcept;
extern "C" void mkw_switch_renderer_shutdown() noexcept;
extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept;
extern "C" MkwSwitchRendererDiagnostics mkw_switch_renderer_diagnostics_snapshot() noexcept;
