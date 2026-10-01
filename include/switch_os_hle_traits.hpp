#pragma once

#include "abi_bridge.h"
#include "memory.h"
#include "switch_thread_hle_traits.hpp"
#include "isa/ppc_isa_int.h"

#include <cstdint>

extern "C" void mkw_switch_hle_os_send_message(CpuContext* cpu) noexcept;
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

// OSSendMessage (PAL 0x801A735C). Pinned WiiCompiled disables interrupts,
// appends to the ring buffer when space exists, wakes receivers, and returns
// success. A full non-blocking send returns 0; a full blocking send parks on
// the embedded send OSThreadQueue through OSSleepThread and retries.
template <>
struct KnownNativeCpuCall<0x801A735Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_send_message(cpu);
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

namespace mkw::switch_os_alarm_hle {

constexpr std::uint32_t kSystemTimeBaseHi = 0x800030D8u;
constexpr std::uint32_t kSystemTimeBaseLo = 0x800030DCu;
constexpr std::uint32_t kAlarmQueueOffsetFromR13 = 0x6360u;

constexpr std::uint32_t kAlarmHandlerOffset = 0x00u;
constexpr std::uint32_t kAlarmFireHiOffset = 0x08u;
constexpr std::uint32_t kAlarmFireLoOffset = 0x0Cu;
constexpr std::uint32_t kAlarmPrevOffset = 0x10u;
constexpr std::uint32_t kAlarmNextOffset = 0x14u;
constexpr std::uint32_t kAlarmPeriodHiOffset = 0x18u;
constexpr std::uint32_t kAlarmPeriodLoOffset = 0x1Cu;
constexpr std::uint32_t kAlarmBeginHiOffset = 0x20u;
constexpr std::uint32_t kAlarmBeginLoOffset = 0x24u;
constexpr std::uint32_t kAlarmSize = 0x2Cu;

inline std::uint64_t JoinU64(std::uint32_t hi, std::uint32_t lo) noexcept {
    return (static_cast<std::uint64_t>(hi) << 32) | lo;
}

inline void WriteU64(std::uint32_t addr, std::uint64_t value) {
    Memory::Write32(addr, static_cast<std::uint32_t>(value >> 32));
    Memory::Write32(addr + 4u, static_cast<std::uint32_t>(value));
}

inline std::uint64_t ReadU64(std::uint32_t addr) {
    return JoinU64(Memory::Read32(addr), Memory::Read32(addr + 4u));
}

inline std::uint64_t ReadStableTimeBase() noexcept {
    while (true) {
        const std::uint32_t hi1 = PPC_Mftbu();
        const std::uint32_t lo = PPC_Mftb();
        const std::uint32_t hi2 = PPC_Mftbu();
        if (hi1 == hi2) {
            return JoinU64(hi1, lo);
        }
    }
}

inline std::uint64_t ReadSystemTimeBase() {
    if (!Memory::IsInitialized() || !Memory::Contains(kSystemTimeBaseHi, 8u)) {
        return 0u;
    }
    return JoinU64(
        Memory::Read32(kSystemTimeBaseHi),
        Memory::Read32(kSystemTimeBaseLo));
}

inline std::uint64_t GetSystemTime() {
    return ReadSystemTimeBase() + ReadStableTimeBase();
}

inline void SanitizeQueue(std::uint32_t queueBase) {
    const std::uint32_t head = Memory::Read32(queueBase);
    const std::uint32_t tail = Memory::Read32(queueBase + 4u);
    if (head == 0u || !Memory::Contains(head, kAlarmSize)) {
        return;
    }

    const std::uint32_t prev = Memory::Read32(head + kAlarmPrevOffset);
    const std::uint32_t next = Memory::Read32(head + kAlarmNextOffset);
    if (head == tail && prev == head && next == head) {
        Memory::Write32(queueBase, 0u);
        Memory::Write32(queueBase + 4u, 0u);
        Memory::Write32(head + kAlarmPrevOffset, 0u);
        Memory::Write32(head + kAlarmNextOffset, 0u);
    }
}

inline void InsertAlarm(
    std::uint32_t queueBase,
    std::uint32_t alarm,
    std::uint64_t requestedFire,
    std::uint32_t handler) {
    const std::uint64_t repeat = ReadU64(alarm + kAlarmPeriodHiOffset);
    std::uint64_t fire = requestedFire;

    if (static_cast<std::int64_t>(repeat) > 0) {
        const std::uint64_t now = GetSystemTime();
        const std::uint64_t begin = ReadU64(alarm + kAlarmBeginHiOffset);
        fire = begin;
        if (static_cast<std::int64_t>(begin) < static_cast<std::int64_t>(now)) {
            fire += repeat * (((now - begin) / repeat) + 1u);
        }
    }

    Memory::Write32(alarm + kAlarmHandlerOffset, handler);
    WriteU64(alarm + kAlarmFireHiOffset, fire);

    std::uint32_t prev = 0u;
    std::uint32_t cur = Memory::Read32(queueBase);

    while (cur != 0u) {
        if (!Memory::Contains(cur, kAlarmSize)) {
            return;
        }

        const std::uint64_t curFire = ReadU64(cur + kAlarmFireHiOffset);
        if (static_cast<std::int64_t>(curFire) > static_cast<std::int64_t>(fire)) {
            break;
        }

        prev = cur;
        cur = Memory::Read32(cur + kAlarmNextOffset);
    }

    if (prev == 0u) {
        Memory::Write32(alarm + kAlarmPrevOffset, 0u);
        Memory::Write32(alarm + kAlarmNextOffset, cur);
        if (cur != 0u) {
            Memory::Write32(cur + kAlarmPrevOffset, alarm);
        } else {
            Memory::Write32(queueBase + 4u, alarm);
        }
        Memory::Write32(queueBase, alarm);
        return;
    }

    Memory::Write32(alarm + kAlarmPrevOffset, prev);
    Memory::Write32(alarm + kAlarmNextOffset, cur);
    Memory::Write32(prev + kAlarmNextOffset, alarm);
    if (cur != 0u) {
        Memory::Write32(cur + kAlarmPrevOffset, alarm);
    } else {
        Memory::Write32(queueBase + 4u, alarm);
    }
}

inline thread_local bool gProcessingDueAlarms = false;

inline void ProcessDueAlarms(CpuContext* cpu, int maxToProcess) noexcept {
    constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
    constexpr std::uint32_t kSchedulerDisableCountAddr = 0x80386918u;

    if (!cpu || maxToProcess <= 0 || gProcessingDueAlarms ||
        !Memory::IsInitialized() ||
        cpu->gpr[13] < kAlarmQueueOffsetFromR13) {
        return;
    }

    const std::uint32_t queueBase =
        cpu->gpr[13] - kAlarmQueueOffsetFromR13;
    if (!Memory::Contains(queueBase, 8u)) {
        return;
    }

    gProcessingDueAlarms = true;
    try {
        SanitizeQueue(queueBase);
        for (int i = 0; i < maxToProcess; ++i) {
            const std::uint32_t alarm = Memory::Read32(queueBase);
            if (alarm == 0u || !Memory::Contains(alarm, kAlarmSize)) {
                break;
            }

            const std::uint64_t fire =
                ReadU64(alarm + kAlarmFireHiOffset);
            if (GetSystemTime() < fire) {
                break;
            }

            const std::uint32_t handler =
                Memory::Read32(alarm + kAlarmHandlerOffset);
            const std::uint32_t next =
                Memory::Read32(alarm + kAlarmNextOffset);

            Memory::Write32(queueBase, next);
            if (next == 0u) {
                Memory::Write32(queueBase + 4u, 0u);
            } else if (Memory::Contains(next, kAlarmSize)) {
                Memory::Write32(next + kAlarmPrevOffset, 0u);
            }

            Memory::Write32(alarm + kAlarmPrevOffset, 0u);
            Memory::Write32(alarm + kAlarmNextOffset, 0u);
            Memory::Write32(alarm + kAlarmHandlerOffset, 0u);

            const std::uint64_t repeat =
                ReadU64(alarm + kAlarmPeriodHiOffset);
            if (repeat != 0u) {
                InsertAlarm(queueBase, alarm, 0u, handler);
            }

            if (handler != 0u) {
                CpuContext callbackCpu = *cpu;
                callbackCpu.gpr[3] = alarm;
                callbackCpu.gpr[4] =
                    Memory::Contains(kOSCurrentContextAddr, 4u)
                    ? Memory::Read32(kOSCurrentContextAddr)
                    : 0u;

                std::uint32_t disableCount = 0u;
                const bool hasDisableCount =
                    Memory::Contains(kSchedulerDisableCountAddr, 4u);
                if (hasDisableCount) {
                    disableCount =
                        Memory::Read32(kSchedulerDisableCountAddr);
                    Memory::Write32(
                        kSchedulerDisableCountAddr,
                        disableCount + 1u);
                }

                try {
                    CpuContextScope scope(&callbackCpu);
                    InvokeIndirectCpu(handler, &callbackCpu);
                } catch (...) {
                    if (hasDisableCount) {
                        Memory::Write32(
                            kSchedulerDisableCountAddr,
                            disableCount);
                    }
                    throw;
                }

                if (hasDisableCount) {
                    Memory::Write32(
                        kSchedulerDisableCountAddr,
                        disableCount);
                }
            }
        }
    } catch (...) {
        // Keep malformed guest alarm state contained at the HLE boundary.
    }
    gProcessingDueAlarms = false;
}


} // namespace mkw::switch_os_alarm_hle

// OSSetPeriodicAlarm (PAL 0x801A08E0). Real Switch hardware reaches this after
// crossing the fourth exact GXInitTexObjLOD descriptor. Pinned WiiCompiled
// preserves the RVL guest alarm fields and sorted queue insertion while its
// PPCMtdec host boundary is stubbed. Mirror exactly that guest-visible state;
// do not invent a host timer, alarm pump, or neighboring alarm API.
template <>
struct KnownNativeCpuCall<0x801A08E0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t alarm = cpu->gpr[3];
        const std::uint32_t startHi = cpu->gpr[5];
        const std::uint32_t startLo = cpu->gpr[6];
        const std::uint32_t periodHi = cpu->gpr[7];
        const std::uint32_t periodLo = cpu->gpr[8];
        const std::uint32_t handler = cpu->gpr[9];

        const std::uint32_t r13 = cpu->gpr[13];
        if (alarm == 0u || r13 < mkw::switch_os_alarm_hle::kAlarmQueueOffsetFromR13) {
            return;
        }

        const std::uint32_t queueBase =
            r13 - mkw::switch_os_alarm_hle::kAlarmQueueOffsetFromR13;

        mkw_switch_hle_os_disable_interrupts(cpu);
        const std::uint32_t irqState = cpu->gpr[3];

        try {
            if (!Memory::IsInitialized() ||
                !Memory::Contains(alarm, mkw::switch_os_alarm_hle::kAlarmSize) ||
                !Memory::Contains(queueBase, 8u)) {
                cpu->gpr[3] = irqState;
                mkw_switch_hle_os_restore_interrupts(cpu);
                return;
            }

            mkw::switch_os_alarm_hle::SanitizeQueue(queueBase);

            mkw::switch_os_alarm_hle::WriteU64(
                alarm + mkw::switch_os_alarm_hle::kAlarmPeriodHiOffset,
                mkw::switch_os_alarm_hle::JoinU64(periodHi, periodLo));

            const std::uint64_t begin =
                mkw::switch_os_alarm_hle::ReadSystemTimeBase() +
                mkw::switch_os_alarm_hle::JoinU64(startHi, startLo);
            mkw::switch_os_alarm_hle::WriteU64(
                alarm + mkw::switch_os_alarm_hle::kAlarmBeginHiOffset,
                begin);

            mkw::switch_os_alarm_hle::InsertAlarm(
                queueBase,
                alarm,
                0u,
                handler);
        } catch (...) {
            // Keep guest-memory faults contained at the native HLE boundary,
            // matching the rest of the Switch OS bridge.
        }

        cpu->gpr[3] = irqState;
        mkw_switch_hle_os_restore_interrupts(cpu);
    }
};

// RFLiIsWorking (PAL 0x800BD860). RFLInitRes polls this while Mii-library
// work completes through the guest alarm queue. Pinned WiiCompiled services up
// to 32 due alarms before reading manager + 0x1B34; mirror that behavior using
// the Switch alarm queue/indirect-dispatch primitives and keep callback register
// writes isolated from the interrupted translated caller.
template <>
struct KnownNativeCpuCall<0x800BD860u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        mkw::switch_os_alarm_hle::ProcessDueAlarms(cpu, 32);

        constexpr std::uint32_t kRflManagerPtrAddr = 0x80386298u;
        constexpr std::uint32_t kWorkingFlagOffset = 0x1B34u;

        std::uint32_t working = 0u;
        try {
            if (Memory::IsInitialized() &&
                Memory::Contains(kRflManagerPtrAddr, 4u)) {
                const std::uint32_t manager =
                    Memory::Read32(kRflManagerPtrAddr);
                if (manager != 0u &&
                    Memory::Contains(
                        manager + kWorkingFlagOffset,
                        4u)) {
                    working =
                        Memory::Read32(
                            manager + kWorkingFlagOffset);
                }
            }
        } catch (...) {
            working = 0u;
        }
        cpu->gpr[3] = working;
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
#include "switch_dvd_read_hle_traits.hpp"
#include "switch_dvd_low_hle_traits.hpp"
#include "switch_egg_decomp_hle_traits.hpp"
#include "switch_ios_kd_hle_traits.hpp"
#include "switch_esp_hle_traits.hpp"
