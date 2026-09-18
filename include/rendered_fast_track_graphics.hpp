#pragma once

struct CpuContext;

extern "C" bool mkw_switch_renderer_initialize() noexcept;
extern "C" void mkw_switch_renderer_shutdown() noexcept;
extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept;
