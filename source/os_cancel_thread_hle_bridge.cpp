#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {

constexpr std::uint32_t kOsCancelThreadAddress = 0x801AA1D4u;
constexpr std::uint32_t kObservedThread = 0x901187C0u;
constexpr std::uint32_t kObservedQueue = 0x90113730u;
constexpr std::uint32_t kObservedListPrev = 0x90112660u;
constexpr std::uint32_t kDefaultThread = 0x80347498u;

constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;
constexpr std::uint32_t kThreadListHeadAddr = 0x800000DCu;
constexpr std::uint32_t kThreadListTailAddr = 0x800000E0u;
constexpr std::uint32_t kSchedulerReschedCounterAddr = 0x8038691Cu;
constexpr std::uint32_t kSchedulerPendingFlagAddr = 0x80386920u;

constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadAttrOffset = 0x2CAu;
constexpr std::uint32_t kThreadSuspendOffset = 0x2CCu;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;
constexpr std::uint32_t kThreadNextOffset = 0x2E0u;
constexpr std::uint32_t kThreadPrevOffset = 0x2E4u;
constexpr std::uint32_t kThreadJoinQueueOffset = 0x2E8u;
constexpr std::uint32_t kThreadMutexOffset = 0x2F0u;
constexpr std::uint32_t kThreadOwnedMutexHeadOffset = 0x2F4u;
constexpr std::uint32_t kThreadOwnedMutexTailOffset = 0x2F8u;
constexpr std::uint32_t kThreadListNextOffset = 0x2FCu;
constexpr std::uint32_t kThreadListPrevOffset = 0x300u;
constexpr std::uint32_t kThreadSize = 0x318u;

constexpr std::uint16_t kObservedState = 4u;
constexpr std::uint16_t kObservedAttr = 0x0001u;
constexpr std::int32_t kObservedPriority = 20;
constexpr std::uint32_t kObservedPending = 0x02000000u;

[[noreturn]] void AbortUnproven(CpuContext* cpu, const char* kind) noexcept {
    if (cpu) {
        cpu->gpr[3] = kObservedThread;
    }
    mkw_switch_report_unsupported_translated_dispatch(
        kind,
        kOsCancelThreadAddress,
        cpu);
    std::abort();
}

[[noreturn]] void RestoreAndAbort(
    CpuContext* cpu,
    std::uint32_t irqState,
    const char* kind) noexcept {
    if (cpu) {
        cpu->gpr[3] = irqState;
        mkw_switch_hle_os_restore_interrupts(cpu);
    }
    AbortUnproven(cpu, kind);
}

bool Mapped(std::uint32_t address, std::uint32_t size) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, size);
}

bool ExactObservedState() {
    if (!Mapped(kObservedThread, kThreadSize) ||
        !Mapped(kObservedQueue, 8u) ||
        !Mapped(kObservedListPrev + kThreadListNextOffset, 4u) ||
        !Mapped(kThreadListHeadAddr, 4u) ||
        !Mapped(kThreadListTailAddr, 4u) ||
        !Mapped(kSchedulerReschedCounterAddr, 4u) ||
        !Mapped(kSchedulerPendingFlagAddr, 4u) ||
        !Mapped(kOSCurrentContextAddr, 4u) ||
        !Mapped(kOSRunningContextAddr, 4u)) {
        return false;
    }

    return Memory::Read16(kObservedThread + kThreadStateOffset) == kObservedState &&
           Memory::Read16(kObservedThread + kThreadAttrOffset) == kObservedAttr &&
           static_cast<std::int32_t>(
               Memory::Read32(kObservedThread + kThreadSuspendOffset)) == 0 &&
           static_cast<std::int32_t>(
               Memory::Read32(kObservedThread + kThreadPriorityOffset)) ==
               kObservedPriority &&
           Memory::Read32(kObservedThread + kThreadQueueOffset) == kObservedQueue &&
           Memory::Read32(kObservedThread + kThreadNextOffset) == 0u &&
           Memory::Read32(kObservedThread + kThreadPrevOffset) == 0u &&
           Memory::Read32(kObservedQueue) == kObservedThread &&
           Memory::Read32(kObservedQueue + 4u) == kObservedThread &&
           Memory::Read32(kObservedThread + kThreadJoinQueueOffset) == 0u &&
           Memory::Read32(kObservedThread + kThreadJoinQueueOffset + 4u) == 0u &&
           Memory::Read32(kObservedThread + kThreadMutexOffset) == 0u &&
           Memory::Read32(kObservedThread + kThreadOwnedMutexHeadOffset) == 0u &&
           Memory::Read32(kObservedThread + kThreadOwnedMutexTailOffset) == 0u &&
           Memory::Read32(kObservedThread + kThreadListNextOffset) == 0u &&
           Memory::Read32(kObservedThread + kThreadListPrevOffset) ==
               kObservedListPrev &&
           Memory::Read32(kObservedListPrev + kThreadListNextOffset) ==
               kObservedThread &&
           Memory::Read32(kThreadListHeadAddr) == kDefaultThread &&
           Memory::Read32(kThreadListTailAddr) == kObservedThread &&
           Memory::Read32(kSchedulerReschedCounterAddr) == 1u &&
           Memory::Read32(kSchedulerPendingFlagAddr) == kObservedPending &&
           Memory::Read32(kOSCurrentContextAddr) == kDefaultThread &&
           Memory::Read32(kOSRunningContextAddr) == kDefaultThread &&
           mkw::switch_guest_fiber::available() &&
           mkw::switch_guest_fiber::has(kObservedThread) &&
           mkw::switch_guest_fiber::current_thread() == kDefaultThread &&
           mkw::switch_guest_fiber::can_terminate_non_current(kObservedThread);
}

} // namespace

extern "C" void mkw_switch_hle_os_cancel_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t threadPtr = cpu->gpr[3];

    mkw_switch_set_fast_track_stage("RMCP01_OS_CANCEL_THREAD");

    if (threadPtr != kObservedThread) {
        AbortUnproven(cpu, "OSCANCELTHREAD_UNPROVEN_THREAD");
    }

    try {
        if (!ExactObservedState()) {
            AbortUnproven(cpu, "OSCANCELTHREAD_UNPROVEN_STATE");
        }
    } catch (...) {
        AbortUnproven(cpu, "OSCANCELTHREAD_GUEST_MEMORY");
    }

    mkw_switch_hle_os_disable_interrupts(cpu);
    const std::uint32_t irqState = cpu->gpr[3];

    try {
        // WAITING path: the observed wait queue contains this thread only.
        Memory::Write32(kObservedQueue, 0u);
        Memory::Write32(kObservedQueue + 4u, 0u);
        Memory::Write32(threadPtr + kThreadQueueOffset, 0u);
        Memory::Write32(threadPtr + kThreadNextOffset, 0u);
        Memory::Write32(threadPtr + kThreadPrevOffset, 0u);

        // Pinned TerminateThreadCommon first clears the guest OSContext.
        cpu->gpr[3] = threadPtr;
        InvokeDirectCpu<0x801A2098u>(cpu); // OSClearContext

        // attr=detached and this thread is the global-list tail.
        Memory::Write32(kObservedListPrev + kThreadListNextOffset, 0u);
        Memory::Write32(kThreadListTailAddr, kObservedListPrev);
        Memory::Write32(threadPtr + kThreadListNextOffset, 0u);
        Memory::Write32(threadPtr + kThreadListPrevOffset, 0u);

        Memory::Write16(threadPtr + kThreadStateOffset, 0u);

        // Hardware proves the target fiber exists but is not current, so its
        // HostContext can be destroyed immediately without touching this stack.
        if (!mkw::switch_guest_fiber::terminate_non_current(threadPtr)) {
            RestoreAndAbort(cpu, irqState, "OSCANCELTHREAD_FIBER_TERMINATION");
        }
        mkw_switch_set_fast_track_stage("RMCP01_OS_CANCEL_THREAD");

        // The exact observed owned-mutex list is empty, so pinned
        // __OSUnlockAllMutex has no guest-visible work on this path.

        // Pinned TerminateThreadCommon still wakes joiners; the exact observed
        // queue is empty, so reuse the proven wake bridge for the no-op queue.
        cpu->gpr[3] = threadPtr + kThreadJoinQueueOffset;
        mkw_switch_hle_os_wakeup_thread(cpu);

        if (Memory::Read32(kSchedulerReschedCounterAddr) != 0u) {
            cpu->gpr[3] = 0u;
            InvokeDirectCpu<0x801A9C08u>(cpu); // SelectThread(0)
        }
    } catch (...) {
        RestoreAndAbort(cpu, irqState, "OSCANCELTHREAD_GUEST_MUTATION");
    }

    cpu->gpr[3] = irqState;
    mkw_switch_hle_os_restore_interrupts(cpu);
}

#endif
