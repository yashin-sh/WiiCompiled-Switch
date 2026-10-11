#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdlib>

// Minimal Horizon port of the pinned WiiCompiled OSMutex native overrides.
// The startup path observed on real hardware is single-threaded/uncontended;
// preserve the exact owner/count/held-mutex bookkeeping for that path and for
// recursive locking. Contended sleep/wakeup/priority-inheritance requires the
// scheduler/fiber HLE and is deliberately kept as a durable blocker instead of
// being approximated incorrectly.
namespace mkw::switch_mutex_hle {

constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadBasePriorityOffset = 0x2D4u;
constexpr std::uint32_t kThreadMutexQueueOffset = 0x2F4u;
constexpr std::uint32_t kThreadMutexTailOffset = 0x2F8u;

constexpr std::uint32_t kMutexWaitQueueHeadOffset = 0x00u;
constexpr std::uint32_t kMutexWaitQueueTailOffset = 0x04u;
constexpr std::uint32_t kMutexOwnerOffset = 0x08u;
constexpr std::uint32_t kMutexCountOffset = 0x0Cu;
constexpr std::uint32_t kMutexThreadNextOffset = 0x10u;
constexpr std::uint32_t kMutexThreadPrevOffset = 0x14u;

inline std::uint32_t DisableInterrupts(CpuContext* cpu) noexcept {
    mkw_switch_hle_os_disable_interrupts(cpu);
    return cpu ? cpu->gpr[3] : 0u;
}

inline void RestoreInterrupts(CpuContext* cpu, std::uint32_t state) noexcept {
    if (!cpu) {
        return;
    }
    cpu->gpr[3] = state;
    mkw_switch_hle_os_restore_interrupts(cpu);
}

[[noreturn]] inline void AbortMutexBoundary(
    const char* kind,
    std::uint32_t target,
    std::uint32_t mutex,
    CpuContext* cpu,
    std::uint32_t irqState) {
    RestoreInterrupts(cpu, irqState);
    if (cpu) {
        cpu->gpr[3] = mutex;
    }
    mkw_switch_report_unsupported_translated_dispatch(kind, target, cpu);
    std::abort();
}

inline bool Mapped32(std::uint32_t address) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, sizeof(std::uint32_t));
}

inline void LinkMutexToThread(
    std::uint32_t thread,
    std::uint32_t mutex,
    CpuContext* cpu,
    std::uint32_t irqState) {
    const std::uint32_t tailAddr = thread + kThreadMutexTailOffset;
    const std::uint32_t headAddr = thread + kThreadMutexQueueOffset;
    if (!Mapped32(tailAddr) || !Mapped32(headAddr) ||
        !Mapped32(mutex + kMutexThreadNextOffset) ||
        !Mapped32(mutex + kMutexThreadPrevOffset)) {
        AbortMutexBoundary(
            "OSMUTEX_BAD_GUEST_STATE", 0x801A7EE4u, mutex, cpu, irqState);
    }

    const std::uint32_t tail = Memory::Read32(tailAddr);
    if (tail == 0u) {
        Memory::Write32(headAddr, mutex);
    } else {
        if (!Mapped32(tail + kMutexThreadNextOffset)) {
            AbortMutexBoundary(
                "OSMUTEX_BAD_HELD_LIST", 0x801A7EE4u, mutex, cpu, irqState);
        }
        Memory::Write32(tail + kMutexThreadNextOffset, mutex);
    }

    Memory::Write32(mutex + kMutexThreadPrevOffset, tail);
    Memory::Write32(mutex + kMutexThreadNextOffset, 0u);
    Memory::Write32(tailAddr, mutex);
}

inline void UnlinkMutexFromThread(
    std::uint32_t thread,
    std::uint32_t mutex,
    CpuContext* cpu,
    std::uint32_t irqState) {
    const std::uint32_t headAddr = thread + kThreadMutexQueueOffset;
    const std::uint32_t tailAddr = thread + kThreadMutexTailOffset;
    if (!Mapped32(headAddr) || !Mapped32(tailAddr) ||
        !Mapped32(mutex + kMutexThreadNextOffset) ||
        !Mapped32(mutex + kMutexThreadPrevOffset)) {
        AbortMutexBoundary(
            "OSMUTEX_BAD_GUEST_STATE", 0x801A7FC0u, mutex, cpu, irqState);
    }

    const std::uint32_t next = Memory::Read32(mutex + kMutexThreadNextOffset);
    const std::uint32_t prev = Memory::Read32(mutex + kMutexThreadPrevOffset);

    if (next == 0u) {
        Memory::Write32(tailAddr, prev);
    } else {
        if (!Mapped32(next + kMutexThreadPrevOffset)) {
            AbortMutexBoundary(
                "OSMUTEX_BAD_HELD_LIST", 0x801A7FC0u, mutex, cpu, irqState);
        }
        Memory::Write32(next + kMutexThreadPrevOffset, prev);
    }

    if (prev == 0u) {
        Memory::Write32(headAddr, next);
    } else {
        if (!Mapped32(prev + kMutexThreadNextOffset)) {
            AbortMutexBoundary(
                "OSMUTEX_BAD_HELD_LIST", 0x801A7FC0u, mutex, cpu, irqState);
        }
        Memory::Write32(prev + kMutexThreadNextOffset, next);
    }
}

} // namespace mkw::switch_mutex_hle

// OSLockMutex (PAL 0x801A7EE4). Pinned WiiCompiled native-overrides this SDK
// primitive. Match its uncontended and recursive paths exactly: disable guest
// interrupts, acquire against OSRunningContext, increment recursion count, and
// link a first acquisition into the thread's held-mutex list.
template <>
struct KnownNativeCpuCall<0x801A7EE4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) {
        using namespace mkw::switch_mutex_hle;
        if (!cpu || !Memory::IsInitialized()) {
            return;
        }

        const std::uint32_t mutex = cpu->gpr[3];
        if (mutex == 0u) {
            return;
        }

        const std::uint32_t irqState = DisableInterrupts(cpu);
        if (!Mapped32(kOSRunningContextAddr) ||
            !Mapped32(mutex + kMutexOwnerOffset) ||
            !Mapped32(mutex + kMutexCountOffset)) {
            AbortMutexBoundary(
                "OSLOCKMUTEX_BAD_GUEST_STATE", 0x801A7EE4u, mutex, cpu, irqState);
        }

        const std::uint32_t currentThread = Memory::Read32(kOSRunningContextAddr);
        if (currentThread == 0u) {
            RestoreInterrupts(cpu, irqState);
            return;
        }

        const std::uint32_t owner = Memory::Read32(mutex + kMutexOwnerOffset);
        if (owner == 0u) {
            Memory::Write32(mutex + kMutexOwnerOffset, currentThread);
            const std::uint32_t count = Memory::Read32(mutex + kMutexCountOffset);
            Memory::Write32(mutex + kMutexCountOffset, count + 1u);
            LinkMutexToThread(currentThread, mutex, cpu, irqState);
        } else if (owner == currentThread) {
            const std::uint32_t count = Memory::Read32(mutex + kMutexCountOffset);
            Memory::Write32(mutex + kMutexCountOffset, count + 1u);
        } else {
            // Pinned WiiCompiled promotes the owner and sleeps the current guest
            // thread here. Do not fake that scheduler transition on Horizon.
            AbortMutexBoundary(
                "OSLOCKMUTEX_CONTENDED", 0x801A7EE4u, mutex, cpu, irqState);
        }

        RestoreInterrupts(cpu, irqState);
    }
};

// OSUnlockMutex (PAL 0x801A7FC0) is the paired pinned native override. The
// no-waiter/no-priority-inheritance path is enough for early single-threaded
// startup and keeps the held-mutex list consistent. If wakeup or inherited
// priority repair becomes necessary, retain that as an explicit blocker until
// the scheduler HLE is ported.
template <>
struct KnownNativeCpuCall<0x801A7FC0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) {
        using namespace mkw::switch_mutex_hle;
        if (!cpu || !Memory::IsInitialized()) {
            return;
        }

        const std::uint32_t mutex = cpu->gpr[3];
        if (mutex == 0u) {
            return;
        }

        const std::uint32_t irqState = DisableInterrupts(cpu);
        if (!Mapped32(kOSRunningContextAddr) ||
            !Mapped32(mutex + kMutexWaitQueueHeadOffset) ||
            !Mapped32(mutex + kMutexWaitQueueTailOffset) ||
            !Mapped32(mutex + kMutexOwnerOffset) ||
            !Mapped32(mutex + kMutexCountOffset)) {
            AbortMutexBoundary(
                "OSUNLOCKMUTEX_BAD_GUEST_STATE", 0x801A7FC0u, mutex, cpu, irqState);
        }

        const std::uint32_t currentThread = Memory::Read32(kOSRunningContextAddr);
        if (currentThread == 0u ||
            Memory::Read32(mutex + kMutexOwnerOffset) != currentThread) {
            RestoreInterrupts(cpu, irqState);
            return;
        }

        const std::uint32_t count = Memory::Read32(mutex + kMutexCountOffset);
        if (count == 0u) {
            AbortMutexBoundary(
                "OSUNLOCKMUTEX_BAD_COUNT", 0x801A7FC0u, mutex, cpu, irqState);
        }

        if (count == 1u) {
            const std::uint32_t waitHead =
                Memory::Read32(mutex + kMutexWaitQueueHeadOffset);
            const std::uint32_t waitTail =
                Memory::Read32(mutex + kMutexWaitQueueTailOffset);
            if (waitHead != 0u || waitTail != 0u) {
                AbortMutexBoundary(
                    "OSUNLOCKMUTEX_WAITERS", 0x801A7FC0u, mutex, cpu, irqState);
            }

            const std::uint32_t priorityAddr = currentThread + kThreadPriorityOffset;
            const std::uint32_t basePriorityAddr =
                currentThread + kThreadBasePriorityOffset;
            if (!Mapped32(priorityAddr) || !Mapped32(basePriorityAddr)) {
                AbortMutexBoundary(
                    "OSUNLOCKMUTEX_BAD_THREAD", 0x801A7FC0u, mutex, cpu, irqState);
            }

            const auto priority = static_cast<std::int32_t>(Memory::Read32(priorityAddr));
            const auto basePriority =
                static_cast<std::int32_t>(Memory::Read32(basePriorityAddr));
            if (priority < basePriority) {
                AbortMutexBoundary(
                    "OSUNLOCKMUTEX_INHERITED_PRIORITY",
                    0x801A7FC0u,
                    mutex,
                    cpu,
                    irqState);
            }
        }

        const std::uint32_t newCount = count - 1u;
        Memory::Write32(mutex + kMutexCountOffset, newCount);
        if (newCount == 0u) {
            UnlinkMutexFromThread(currentThread, mutex, cpu, irqState);
            Memory::Write32(mutex + kMutexOwnerOffset, 0u);
        }

        RestoreInterrupts(cpu, irqState);
    }
};
