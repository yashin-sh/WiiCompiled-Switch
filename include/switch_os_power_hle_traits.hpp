#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>

// OSSetPowerCallback (PAL 0x801AB75C). Pinned WiiCompiled keeps the SDK-visible
// SDA callback bookkeeping but stubs the real /dev/stm event registration:
// disable interrupts, return the previous callback (NULL when it was the SDK
// default), install the new callback or restore the SDK default for NULL, mark
// the STM handler active, then restore the caller's interrupt state.
template <>
struct KnownNativeCpuCall<0x801AB75Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t newCallback = cpu->gpr[3];
        const std::uint32_t r13 = cpu->gpr[13];
        constexpr std::uint32_t kDefaultCallback = 0x801ABC0Cu;
        constexpr std::uint32_t kCallbackOffset = 0x62B8u;
        constexpr std::uint32_t kHandlerActiveOffset = 0x62C0u;

        mkw_switch_hle_os_disable_interrupts(cpu);
        const std::uint32_t irqState = cpu->gpr[3];

        std::uint32_t oldCallback = kDefaultCallback;
        const std::uint32_t callbackAddr = r13 - kCallbackOffset;
        const std::uint32_t handlerActiveAddr = r13 - kHandlerActiveOffset;

        // Memory::* aborts on unmapped ranges in the current Switch slice, so
        // preflight each upstream access explicitly. Preserve upstream partial
        // side effects: if the callback slot is valid, update it even when the
        // later handler-active slot is unavailable.
        if (Memory::Contains(callbackAddr, 4u)) {
            oldCallback = Memory::Read32(callbackAddr);
            const std::uint32_t callbackToWrite =
                newCallback != 0u ? newCallback : kDefaultCallback;
            Memory::Write32(callbackAddr, callbackToWrite);

            if (Memory::Contains(handlerActiveAddr, 4u)) {
                if (Memory::Read32(handlerActiveAddr) == 0u) {
                    Memory::Write32(handlerActiveAddr, 1u);
                }
            }
        }

        cpu->gpr[3] = irqState;
        mkw_switch_hle_os_restore_interrupts(cpu);
        cpu->gpr[3] = oldCallback == kDefaultCallback ? 0u : oldCallback;
    }
};
