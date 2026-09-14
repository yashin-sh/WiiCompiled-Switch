#pragma once

#include "abi_bridge.h"

extern "C" void mkw_switch_hle_vi_init(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_vi_set_black(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_vi_configure(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_vi_flush(CpuContext* cpu) noexcept;

template <>
struct KnownNativeCpuCall<0x801B94A4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_vi_init(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x801B9294u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_vi_init(cpu);
    }
};

// VIGetDTVStatus (PAL 0x801BAD38). Pinned WiiCompiled deliberately skips the
// Wii VI MMIO read at 0xCC00206E and reports DTV as not ready / disabled.
// This boundary has no additional guest-memory, retrace or renderer effects.
template <>
struct KnownNativeCpuCall<0x801BAD38u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x801BAB2Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_vi_set_black(cpu);
    }
};

// VIConfigure (PAL 0x801B9F6C). Pinned WiiCompiled validates and decodes the
// guest GXRenderModeObj into pending VI state. The host presenter call is not
// part of the Switch fast-track contract while rendering remains headless.
template <>
struct KnownNativeCpuCall<0x801B9F6Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_vi_configure(cpu);
    }
};

// VIFlush (PAL 0x801BA9A4). Pinned WiiCompiled arms pending VI state for the
// next retrace and returns zero. The Switch fast-track records only that guest
// VI bookkeeping; it does not fabricate a retrace or presenter frame.
template <>
struct KnownNativeCpuCall<0x801BA9A4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_vi_flush(cpu);
    }
};
