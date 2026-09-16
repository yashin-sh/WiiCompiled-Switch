#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;
constexpr std::uint32_t kDefaultThreadContextAddr = 0x80347498u;
constexpr std::uint32_t kSchedulerIdleFlagAddr = 0x80386918u;
constexpr std::uint32_t kSchedulerReschedCounterAddr = 0x8038691Cu;

constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;
constexpr std::uint32_t kThreadNextOffset = 0x2E0u;
constexpr std::uint32_t kThreadPrevOffset = 0x2E4u;

constexpr std::uint16_t kThreadStateRunning = 2u;
constexpr std::uint16_t kThreadStateWaiting = 4u;

bool Mapped32(std::uint32_t address) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, sizeof(std::uint32_t));
}

bool Mapped16(std::uint32_t address) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, sizeof(std::uint16_t));
}

void RestoreInterrupts(CpuContext* cpu, std::uint32_t state) noexcept {
    if (!cpu) {
        return;
    }
    cpu->gpr[3] = state;
    mkw_switch_hle_os_restore_interrupts(cpu);
}

void InsertThreadIntoQueueByPriority(std::uint32_t queuePtr,
                                     std::uint32_t threadPtr,
                                     std::int32_t priority) {
    Memory::Write32(threadPtr + kThreadQueueOffset, queuePtr);

    std::uint32_t insertBefore = Memory::Read32(queuePtr);
    while (insertBefore != 0u) {
        const std::int32_t queuedPriority =
            static_cast<std::int32_t>(Memory::Read32(insertBefore + kThreadPriorityOffset));
        if (queuedPriority > priority) {
            break;
        }
        insertBefore = Memory::Read32(insertBefore + kThreadNextOffset);
    }

    if (insertBefore == 0u) {
        const std::uint32_t tail = Memory::Read32(queuePtr + 4u);
        if (tail == 0u) {
            Memory::Write32(queuePtr, threadPtr);
        } else {
            Memory::Write32(tail + kThreadNextOffset, threadPtr);
        }
        Memory::Write32(threadPtr + kThreadPrevOffset, tail);
        Memory::Write32(threadPtr + kThreadNextOffset, 0u);
        Memory::Write32(queuePtr + 4u, threadPtr);
        return;
    }

    Memory::Write32(threadPtr + kThreadNextOffset, insertBefore);
    const std::uint32_t prev = Memory::Read32(insertBefore + kThreadPrevOffset);
    Memory::Write32(insertBefore + kThreadPrevOffset, threadPtr);
    Memory::Write32(threadPtr + kThreadPrevOffset, prev);
    if (prev == 0u) {
        Memory::Write32(queuePtr, threadPtr);
    } else {
        Memory::Write32(prev + kThreadNextOffset, threadPtr);
    }
}

void UnlinkWaitQueueNode(std::uint32_t queuePtr, std::uint32_t threadPtr) {
    const std::uint32_t next = Memory::Read32(threadPtr + kThreadNextOffset);
    const std::uint32_t prev = Memory::Read32(threadPtr + kThreadPrevOffset);

    if (next != 0u) {
        Memory::Write32(next + kThreadPrevOffset, prev);
    } else {
        Memory::Write32(queuePtr + 4u, prev);
    }

    if (prev != 0u) {
        Memory::Write32(prev + kThreadNextOffset, next);
    } else {
        Memory::Write32(queuePtr, next);
    }

    Memory::Write32(threadPtr + kThreadNextOffset, 0u);
    Memory::Write32(threadPtr + kThreadPrevOffset, 0u);
}

} // namespace

extern "C" void mkw_switch_hle_os_sleep_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t queuePtr = cpu->gpr[3];

    if (queuePtr == 0u) {
        return;
    }

    mkw_switch_hle_os_disable_interrupts(cpu);
    const std::uint32_t irqState = cpu->gpr[3];

    try {
        if (!Mapped32(kOSRunningContextAddr) ||
            !Mapped32(kOSCurrentContextAddr) ||
            !Mapped32(kSchedulerIdleFlagAddr) ||
            !Mapped32(kSchedulerReschedCounterAddr) ||
            !Mapped32(queuePtr) ||
            !Mapped32(queuePtr + 4u)) {
            RestoreInterrupts(cpu, irqState);
            return;
        }

        std::uint32_t currentThread = Memory::Read32(kOSRunningContextAddr);
        if (currentThread == 0u) {
            currentThread = kDefaultThreadContextAddr;
            if (!Mapped16(currentThread + kThreadStateOffset) ||
                !Mapped32(currentThread + kThreadPriorityOffset) ||
                !Mapped32(currentThread + kThreadQueueOffset) ||
                !Mapped32(currentThread + kThreadNextOffset) ||
                !Mapped32(currentThread + kThreadPrevOffset)) {
                RestoreInterrupts(cpu, irqState);
                return;
            }

            Memory::Write32(kOSRunningContextAddr, currentThread);
            cpu->gpr[3] = currentThread;
            InvokeDirectCpu<0x801A1E70u>(cpu); // OSSetCurrentContext
        }

        if (!Mapped16(currentThread + kThreadStateOffset) ||
            !Mapped32(currentThread + kThreadPriorityOffset) ||
            !Mapped32(currentThread + kThreadQueueOffset) ||
            !Mapped32(currentThread + kThreadNextOffset) ||
            !Mapped32(currentThread + kThreadPrevOffset)) {
            RestoreInterrupts(cpu, irqState);
            return;
        }

        // Pinned WiiCompiled refuses to park while OSDisableScheduler nesting is
        // non-zero. Leave the thread and wait queue untouched so the caller can
        // retry its blocking condition without double-linking this OSThread.
        if (Memory::Read32(kSchedulerIdleFlagAddr) != 0u) {
            RestoreInterrupts(cpu, irqState);
            return;
        }

        Memory::Write16(currentThread + kThreadStateOffset, kThreadStateWaiting);
        Memory::Write32(currentThread + kThreadQueueOffset, queuePtr);

        const std::int32_t priority =
            static_cast<std::int32_t>(Memory::Read32(currentThread + kThreadPriorityOffset));
        InsertThreadIntoQueueByPriority(queuePtr, currentThread, priority);

        // The wait state now has a matching native continuation when this thread
        // was created through OSCreateThread. SelectThread will switch away from
        // this host stack instead of flattening the continuation into SRR0.
        mkw::switch_guest_fiber::suspend(currentThread);
        Memory::Write32(kSchedulerReschedCounterAddr, 1u);

        cpu->gpr[3] = 0u;
        InvokeDirectCpu<0x801A9C08u>(cpu);

        // SelectThread may deliberately return without switching. Never let the
        // caller keep running while the same OSThread remains WAITING and linked.
        if (Memory::Read16(currentThread + kThreadStateOffset) == kThreadStateWaiting &&
            Memory::Read32(currentThread + kThreadQueueOffset) == queuePtr) {
            UnlinkWaitQueueNode(queuePtr, currentThread);
            Memory::Write32(currentThread + kThreadQueueOffset, 0u);
            Memory::Write16(currentThread + kThreadStateOffset, kThreadStateRunning);
            mkw::switch_guest_fiber::resume(currentThread);
        }
    } catch (...) {
        // Match the pin's contained guest-memory-fault boundary. The normal
        // hardware path is fully mapped; do not manufacture scheduler state on
        // a malformed queue/context.
    }

    RestoreInterrupts(cpu, irqState);
}

#endif
