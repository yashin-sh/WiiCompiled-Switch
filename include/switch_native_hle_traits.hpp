#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>

// Switch-side native/HLE extensions for translated targets that the pinned
// WiiCompiled runtime deliberately excludes from recompilation. Keeping these
// traits outside abi_bridge.h lets blocker-driven HLE coverage grow without
// turning the core ABI seam into a monolithic address catalogue.

// REGISTER_NATIVE_FUNCTION_AS keeps the original guest body translated while
// making a native wrapper the runtime winner. The pinned OSInitAlarm wrapper
// simply forwards to that translated body, so preserve that exact boundary.
extern "C" void func_801A961C(CpuContext* ctx);

template <>
struct KnownNativeCpuCall<0x801A961Cu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            func_801A961C(cpu);
        }
    }
};

// RVL__EXIImm / EXIImm (PAL 0x80167F68). Pinned WiiCompiled reports immediate
// transfers as successful. Read/RW transfers also clear the guest destination
// bytes before returning so callers never consume stale EXI data.
template <>
struct KnownNativeCpuCall<0x80167F68u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t buffer = cpu->gpr[4];
        const std::uint32_t length = cpu->gpr[5];
        const std::uint32_t type = cpu->gpr[6];

        if (type == 0u || type == 2u) {
            try {
                for (std::uint32_t i = 0; i < length; ++i) {
                    Memory::Write8(buffer + i, 0u);
                }
            } catch (...) {
                // Match the pinned host HLE: an unmapped guest buffer is a
                // best-effort write failure, not an EXI transaction failure.
            }
        }

        cpu->gpr[3] = 1u;
    }
};

// The pinned host HLE treats DMA completion, sync, and unlock as immediate
// success while no physical EXI device is emulated on the host.
template <>
struct KnownNativeCpuCall<0x80168288u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x80168380u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x80169260u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};
