#pragma once

#include <cstdint>

struct CpuContext;

extern "C" void mkw_switch_hle_gx_copy_tex(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_dc_range(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_gx_invalidate_copy_destinations(std::uint32_t address, std::uint32_t bytes) noexcept;
extern "C" void mkw_switch_gx_forget_copy_destinations() noexcept;

extern "C" bool mkw_switch_renderer_has_active_frame() noexcept;
