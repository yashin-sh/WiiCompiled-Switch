#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>

// __OSInitSTM (PAL 0x801AB848). Pinned WiiCompiled skips real /dev/stm/* IOS
// handles and publishes the guest-visible STM state directly in the SDA block:
// initialized=1 plus two stable non-zero fake handles. OSResetSystem later
// checks these values, so this boundary is not a no-op.
template <>
struct KnownNativeCpuCall<0x801AB848u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        cpu->gpr[3] = 0u;

        const std::uint32_t r13 = cpu->gpr[13];
        if (r13 == 0u) {
            return;
        }

        constexpr std::uint32_t kInitializedOffset = 0x62CCu;
        constexpr std::uint32_t kImmediateHandleOffset = 0x62C8u;
        constexpr std::uint32_t kEventHookHandleOffset = 0x62C4u;

        const std::uint32_t initializedAddr = r13 - kInitializedOffset;
        const std::uint32_t immediateHandleAddr = r13 - kImmediateHandleOffset;
        const std::uint32_t eventHookHandleAddr = r13 - kEventHookHandleOffset;

        if (!Memory::Contains(initializedAddr, 4u) ||
            !Memory::Contains(immediateHandleAddr, 4u) ||
            !Memory::Contains(eventHookHandleAddr, 4u)) {
            return;
        }

        Memory::Write32(initializedAddr, 1u);
        Memory::Write32(immediateHandleAddr, 0x00535401u);
        Memory::Write32(eventHookHandleAddr, 0x00535402u);

        // Power/reset callback pointers remain unset; the Switch fast-track
        // never fires the Wii STM hardware interrupt that would consume them.
        cpu->gpr[3] = 1u;
    }
};

// Storage initialization is part of the same early OS bootstrap catalogue.
#include "switch_nand_hle_traits.hpp"
