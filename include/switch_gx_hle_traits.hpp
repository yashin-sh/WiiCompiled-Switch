#pragma once

#include "abi_bridge.h"

extern "C" void mkw_switch_hle_gx_init(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_disp_copy_src(CpuContext* cpu) noexcept;

template <>
struct KnownNativeCpuCall<0x8016B850u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_init(cpu);
    }
};

// GXSetDispCopySrc (PAL 0x8016F438). Pinned WiiCompiled narrows r3..r6 to
// u16, records the display-copy source rectangle, and emits the two BP/RAS
// register writes through the GX FIFO. The Switch fast-track keeps that state
// and FIFO contract while the FIFO itself remains the deliberate headless sink.
template <>
struct KnownNativeCpuCall<0x8016F438u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_disp_copy_src(cpu);
    }
};
