#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdlib>

namespace {

constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadSuspendOffset = 0x2CCu;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadBasePriorityOffset = 0x2D4u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;
constexpr std::uint32_t kThreadNextOffset = 0x2E0u;
constexpr std::uint32_t kThreadPrevOffset = 0x2E4u;
constexpr std::uint32_t kThreadMutexOffset = 0x2F0u;
constexpr std::uint32_t kThreadMutexQueueOffset = 0x2F4u;

constexpr std::uint32_t kMutexWaitQueueHeadOffset = 0x00u;
constexpr std::uint32_t kMutexOwnerOffset = 0x08u;
constexpr std::uint32_t kMutexThreadNextOffset = 0x10u;

constexpr std::uint32_t kThreadQueueArrayAddr = 0x803477B0u;
constexpr std::uint32_t kThreadQueueArrayBytes = 0x100u;
constexpr std::uint32_t kSchedulerReschedCounterAddr = 0x8038691Cu;
constexpr std::uint32_t kSchedulerPendingFlagAddr = 0x80386920u;

constexpr std::uint16_t kThreadStateReady = 1u;
constexpr std::uint16_t kThreadStateRunning = 2u;
constexpr std::uint16_t kThreadStateWaiting = 4u;

bool Mapped32(std::uint32_t address) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, sizeof(std::uint32_t));
}

void RestoreInterrupts(CpuContext* cpu, std::uint32_t state) noexcept {
    if (!cpu) {
        return;
    }
    cpu->gpr[3] = state;
    mkw_switch_hle_os_restore_interrupts(cpu);
}

[[noreturn]] void AbortResumeBoundary(const char* kind,
                                      CpuContext* cpu,
                                      std::uint32_t threadPtr,
                                      std::uint32_t irqState) noexcept {
    RestoreInterrupts(cpu, irqState);
    if (cpu) {
        cpu->gpr[3] = threadPtr;
    }
    mkw_switch_report_unsupported_translated_dispatch(kind, 0x801AA58Cu, cpu);
    std::abort();
}

void UpdatePendingMaskForQueue(std::uint32_t queueEntry) {
    if (queueEntry < kThreadQueueArrayAddr ||
        queueEntry >= (kThreadQueueArrayAddr + kThreadQueueArrayBytes) ||
        ((queueEntry - kThreadQueueArrayAddr) % 8u) != 0u) {
        return;
    }

    if (Memory::Read32(queueEntry) != 0u) {
        return;
    }

    const std::uint32_t priority = (queueEntry - kThreadQueueArrayAddr) / 8u;
    const std::uint32_t pending = Memory::Read32(kSchedulerPendingFlagAddr);
    Memory::Write32(kSchedulerPendingFlagAddr, pending & ~(1u << (31u - priority)));
}

void RemoveThreadFromQueue(std::uint32_t threadPtr) {
    const std::uint32_t queuePtr = Memory::Read32(threadPtr + kThreadQueueOffset);
    if (queuePtr == 0u) {
        return;
    }

    const std::uint32_t next = Memory::Read32(threadPtr + kThreadNextOffset);
    const std::uint32_t prev = Memory::Read32(threadPtr + kThreadPrevOffset);

    if (next == 0u) {
        Memory::Write32(queuePtr + 4u, prev);
    } else {
        Memory::Write32(next + kThreadPrevOffset, prev);
    }

    if (prev == 0u) {
        Memory::Write32(queuePtr, next);
    } else {
        Memory::Write32(prev + kThreadNextOffset, next);
    }

    Memory::Write32(threadPtr + kThreadNextOffset, 0u);
    Memory::Write32(threadPtr + kThreadPrevOffset, 0u);
    Memory::Write32(threadPtr + kThreadQueueOffset, 0u);
    UpdatePendingMaskForQueue(queuePtr);
}

std::int32_t ComputeThreadEffectivePriority(std::uint32_t threadPtr) {
    std::int32_t priority =
        static_cast<std::int32_t>(Memory::Read32(threadPtr + kThreadBasePriorityOffset));

    for (std::uint32_t mutexPtr = Memory::Read32(threadPtr + kThreadMutexQueueOffset);
         mutexPtr != 0u;
         mutexPtr = Memory::Read32(mutexPtr + kMutexThreadNextOffset)) {
        const std::uint32_t waiterThread = Memory::Read32(mutexPtr + kMutexWaitQueueHeadOffset);
        if (waiterThread == 0u) {
            continue;
        }

        const std::int32_t waiterPriority =
            static_cast<std::int32_t>(Memory::Read32(waiterThread + kThreadPriorityOffset));
        if (waiterPriority < priority) {
            priority = waiterPriority;
        }
    }

    return priority;
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
    } else {
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

    if (queuePtr >= kThreadQueueArrayAddr &&
        queuePtr < (kThreadQueueArrayAddr + kThreadQueueArrayBytes) &&
        ((queuePtr - kThreadQueueArrayAddr) % 8u) == 0u) {
        const std::uint32_t queueIndex = (queuePtr - kThreadQueueArrayAddr) / 8u;
        const std::uint32_t pending = Memory::Read32(kSchedulerPendingFlagAddr);
        Memory::Write32(kSchedulerPendingFlagAddr, pending | (1u << (31u - queueIndex)));
    }
}

} // namespace

extern "C" void mkw_switch_hle_os_resume_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t threadPtr = cpu->gpr[3];

    if (threadPtr == 0u) {
        cpu->gpr[3] = 0u;
        return;
    }

    mkw_switch_hle_os_disable_interrupts(cpu);
    const std::uint32_t irqState = cpu->gpr[3];

    try {
        if (!Mapped32(threadPtr + kThreadSuspendOffset) ||
            !Memory::Contains(threadPtr + kThreadStateOffset, sizeof(std::uint16_t))) {
            AbortResumeBoundary("OSRESUMETHREAD_BAD_GUEST_STATE", cpu, threadPtr, irqState);
        }

        const std::int32_t suspendCount =
            static_cast<std::int32_t>(Memory::Read32(threadPtr + kThreadSuspendOffset));
        const std::int32_t newSuspend = suspendCount - 1;

        if (newSuspend < 0) {
            Memory::Write32(threadPtr + kThreadSuspendOffset, 0u);
        } else {
            Memory::Write32(threadPtr + kThreadSuspendOffset,
                            static_cast<std::uint32_t>(newSuspend));

            if (newSuspend == 0) {
                const std::uint16_t state = Memory::Read16(threadPtr + kThreadStateOffset);

                if (state == kThreadStateWaiting) {
                    const std::uint32_t mutexPtr = Memory::Read32(threadPtr + kThreadMutexOffset);
                    if (mutexPtr != 0u) {
                        AbortResumeBoundary(
                            "OSRESUMETHREAD_WAITING_MUTEX_PRIORITY",
                            cpu,
                            threadPtr,
                            irqState);
                    }

                    const std::uint32_t queuePtr = Memory::Read32(threadPtr + kThreadQueueOffset);
                    RemoveThreadFromQueue(threadPtr);
                    const std::int32_t priority = ComputeThreadEffectivePriority(threadPtr);
                    if (priority < 0 || priority > 31) {
                        AbortResumeBoundary(
                            "OSRESUMETHREAD_BAD_PRIORITY", cpu, threadPtr, irqState);
                    }
                    Memory::Write32(threadPtr + kThreadPriorityOffset,
                                    static_cast<std::uint32_t>(priority));
                    if (queuePtr != 0u) {
                        InsertThreadIntoQueueByPriority(queuePtr, threadPtr, priority);
                    }
                } else if (state == kThreadStateReady) {
                    const std::int32_t priority = ComputeThreadEffectivePriority(threadPtr);
                    if (priority < 0 || priority > 31) {
                        AbortResumeBoundary(
                            "OSRESUMETHREAD_BAD_PRIORITY", cpu, threadPtr, irqState);
                    }

                    Memory::Write32(threadPtr + kThreadPriorityOffset,
                                    static_cast<std::uint32_t>(priority));
                    const std::uint32_t queueEntry =
                        kThreadQueueArrayAddr + static_cast<std::uint32_t>(priority) * 8u;
                    InsertThreadIntoQueueByPriority(queueEntry, threadPtr, priority);
                    Memory::Write32(kSchedulerReschedCounterAddr, 1u);
                } else if (state == kThreadStateRunning) {
                    // Pinned WiiCompiled has a desktop-fiber race recovery here.
                    // Horizon has no GuestFiberManager state to prove that recovery safe.
                    AbortResumeBoundary(
                        "OSRESUMETHREAD_RUNNING_RECOVERY", cpu, threadPtr, irqState);
                }

                if (Memory::Read32(kSchedulerReschedCounterAddr) != 0u) {
                    // The pin immediately enters SelectThread(0) with interrupts still
                    // disabled. Keep that exact dependency explicit; the scheduler is
                    // the next durable boundary until its own hardware blocker is proven.
                    cpu->gpr[3] = 0u;
                    InvokeDirectCpu<0x801A9C08u>(cpu);
                }
            }
        }

        cpu->gpr[3] = static_cast<std::uint32_t>(suspendCount);
    } catch (...) {
        cpu->gpr[3] = 0u;
    }

    RestoreInterrupts(cpu, irqState);
}

#endif
