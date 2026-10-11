#pragma once

#include "abi_bridge.h"

// ESP_InitLib (PAL 0x801671D0). Pinned WiiCompiled does not open the Wii
// /dev/es device on the host; its native override simply reports success so the
// guest can continue through the title-services bootstrap.
template <>
struct KnownNativeCpuCall<0x801671D0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};

// ESP_CloseLib (PAL 0x80167224). Pinned WiiCompiled has no host /dev/es handle
// to release, so its native override is a no-op that reports success.
template <>
struct KnownNativeCpuCall<0x80167224u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};
