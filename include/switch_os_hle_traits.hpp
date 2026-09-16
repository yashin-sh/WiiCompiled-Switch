#pragma once

#include "abi_bridge.h"
#include "memory.h"
#include "switch_thread_hle_traits.hpp"
#include "isa/ppc_isa_int.h"

#include <cstdint>

extern "C" void mkw_switch_hle_os_receive_message(CpuContext* cpu) noexcept;

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

// OS__InitMessageQueue (PAL 0x801A72FC). Pinned WiiCompiled initializes the
// guest OSMessageQueue in place: clear both embedded OSThreadQueue head/tail
// pairs, publish the caller-provided message-array pointer/count, and reset the
// ring-buffer first/used counters. It returns no value and leaves CpuContext
// registers unchanged.
template <>
struct KnownNativeCpuCall<0x801A72FCu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t queuePtr = cpu->gpr[3];
        if (queuePtr == 0u) {
            return;
        }

        const std::uint32_t msgArrayPtr = cpu->gpr[4];
        const std::uint32_t msgCount = cpu->gpr[5];
        constexpr std::uint32_t kQueueSize = 0x20u;

        try {
            if (!Memory::Contains(queuePtr, kQueueSize)) {
                return;
            }

            Memory::Write32(queuePtr + 0x00u, 0u);
            Memory::Write32(queuePtr + 0x04u, 0u);
            Memory::Write32(queuePtr + 0x08u, 0u);
            Memory::Write32(queuePtr + 0x0Cu, 0u);
            Memory::Write32(queuePtr + 0x10u, msgArrayPtr);
            Memory::Write32(queuePtr + 0x14u, msgCount);
            Memory::Write32(queuePtr + 0x18u, 0u);
            Memory::Write32(queuePtr + 0x1Cu, 0u);
        } catch (...) {
            // Match the pinned HLE boundary: a guest-memory fault is contained
            // inside the native override rather than escaping into the caller.
        }
    }
};

// OSReceiveMessage (PAL 0x801A7424). Pinned WiiCompiled disables interrupts,
// dequeues one ring-buffer message when available, wakes senders, and returns
// success. An empty non-blocking receive returns 0; an empty blocking receive
// parks on the embedded receive OSThreadQueue through OSSleepThread and retries.
// Keep sleep/wakeup as explicit blocker-driven scheduler boundaries.
template <>
struct KnownNativeCpuCall<0x801A7424u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_receive_message(cpu);
    }
};

// OSGetTime (PAL 0x801AAD5C). Pinned WiiCompiled reads the 64-bit Broadway
// time base using the SDK rollover-safe TBU/TBL/TBU sequence and publishes the
// stable high/low words in guest r3:r4. It has no guest-memory side effects.
template <>
struct KnownNativeCpuCall<0x801AAD5Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        while (true) {
            const std::uint32_t hi1 = PPC_Mftbu();
            const std::uint32_t lo = PPC_Mftb();
            const std::uint32_t hi2 = PPC_Mftbu();
            if (hi1 == hi2) {
                cpu->gpr[3] = hi1;
                cpu->gpr[4] = lo;
                return;
            }
        }
    }
};

// SCCheckStatus (PAL 0x801B0220). OSInit polls this while SYSCONF is being
// loaded asynchronously through NAND IPC. Pinned WiiCompiled has no matching
// asynchronous IOS callback pump for this path, so its native override returns
// SC_STATUS_OK (0) immediately to prevent the guest from spinning forever.
template <>
struct KnownNativeCpuCall<0x801B0220u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};

// SCGetEuRgb60Mode (PAL 0x801B1CAC). Pinned WiiCompiled deliberately exposes
// PAL60/RGB60 as the SYSCONF value for PAL builds, so return 1 directly. This
// boundary has no guest-memory or IOS side effects in the pin.
template <>
struct KnownNativeCpuCall<0x801B1CACu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

// SCGetAspectRatio (PAL 0x801B1BE4). Pinned WiiCompiled sources this from the
// runtime widescreen setting with a default of true. The Switch fast-track has
// no runtime-config surface for this setting yet, so mirror the pinned default
// path and report 16:9 directly. This boundary has no guest-memory side effects.
template <>
struct KnownNativeCpuCall<0x801B1BE4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

// Storage/title-service initialization is part of the same early OS bootstrap
// catalogue.
#include "switch_nand_hle_traits.hpp"
#include "switch_nand_open_async_hle_traits.hpp"
#include "switch_nand_read_async_hle_traits.hpp"
#include "switch_nand_close_async_hle_traits.hpp"
#include "switch_dvd_hle_traits.hpp"
#include "switch_dvd_low_hle_traits.hpp"
#include "switch_esp_hle_traits.hpp"
