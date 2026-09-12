#pragma once

#include "abi_bridge.h"

// PPCMthid2 (PAL 0x8012E638). Pinned WiiCompiled preserves HID2 as guest CPU
// state: the native/HLE wrapper copies r3 into CpuContext::hid2. Do not no-op
// this boundary, because the paired PPCMfhid2 path reads the value back later.
template <>
struct KnownNativeCpuCall<0x8012E638u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->hid2 = cpu->gpr[3];
        }
    }
};
