#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <cstdint>
#include <cstdlib>

namespace {

constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;
constexpr std::uint32_t kThreadQueueArrayAddr = 0x803477B0u;
constexpr std::uint32_t kThreadQueueArrayBytes = 0x100u;
constexpr std::uint32_t kSwitchThreadCallbackPtrAddr = 0x80385AE0u;
constexpr std::uint32_t kSchedulerIdleFlagAddr = 0x80386918u;
constexpr std::uint32_t kSchedulerReschedCounterAddr = 0x8038691Cu;
constexpr std::uint32_t kSchedulerPendingFlagAddr = 0x80386920u;

constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;
constexpr std::uint32_t kThreadNextOffset = 0x2E0u;
constexpr std::uint32_t kThreadPrevOffset = 0x2E4u;
constexpr std::uint32_t kContextModeFlagsOffset = 0x1A2u;

constexpr std::uint16_t kThreadStateReady = 1u;
constexpr std::uint16_t kThreadStateRunning = 2u;
constexpr std::uint16_t kThreadStateMoribund = 8u;

bool Mapped32(std::uint32_t address) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, sizeof(std::uint32_t));
}

bool Mapped16(std::uint32_t address) noexcept {
    return Memory::IsInitialized() && Memory::Contains(address, sizeof(std::uint16_t));
}

[[noreturn]] void AbortSelectBoundary(const char* kind, CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(kind, 0x801A9C08u, cpu);
    std::abort();
}

void LinkThreadOnRunQueue(std::uint32_t thread, std::int32_t priority) {
    if (priority < 0 || priority > 31) {
        AbortSelectBoundary("SELECTTHREAD_BAD_PRIORITY", nullptr);
    }

    const std::uint32_t queueEntry =
        kThreadQueueArrayAddr + static_cast<std::uint32_t>(priority) * 8u;
    const std::uint32_t tail = Memory::Read32(queueEntry + 4u);
    if (tail == 0u) {
        Memory::Write32(queueEntry, thread);
    } else {
        Memory::Write32(tail + kThreadNextOffset, thread);
    }

    Memory::Write32(thread + kThreadPrevOffset, tail);
    Memory::Write32(thread + kThreadNextOffset, 0u);
    Memory::Write32(queueEntry + 4u, thread);
    Memory::Write32(thread + kThreadQueueOffset, queueEntry);
}

void MarkRunQueuePending(std::int32_t priority) {
    const std::uint32_t pending = Memory::Read32(kSchedulerPendingFlagAddr);
    Memory::Write32(
        kSchedulerPendingFlagAddr,
        pending | (1u << (31u - static_cast<std::uint32_t>(priority))));
    Memory::Write32(kSchedulerReschedCounterAddr, 1u);
}

std::uint32_t PopThreadQueueHead(std::uint32_t queueAddr, std::uint32_t head) {
    const std::uint32_t next = Memory::Read32(head + kThreadNextOffset);
    if (next == 0u) {
        Memory::Write32(queueAddr + 4u, 0u);
    } else {
        Memory::Write32(next + kThreadPrevOffset, 0u);
    }
    Memory::Write32(queueAddr, next);
    return next;
}

void RemoveThreadFromQueue(std::uint32_t thread) {
    const std::uint32_t queue = Memory::Read32(thread + kThreadQueueOffset);
    if (queue == 0u) {
        return;
    }

    const std::uint32_t next = Memory::Read32(thread + kThreadNextOffset);
    const std::uint32_t prev = Memory::Read32(thread + kThreadPrevOffset);
    if (next == 0u) {
        Memory::Write32(queue + 4u, prev);
    } else {
        Memory::Write32(next + kThreadPrevOffset, prev);
    }
    if (prev == 0u) {
        Memory::Write32(queue, next);
    } else {
        Memory::Write32(prev + kThreadNextOffset, next);
    }

    Memory::Write32(thread + kThreadQueueOffset, 0u);
    Memory::Write32(thread + kThreadNextOffset, 0u);
    Memory::Write32(thread + kThreadPrevOffset, 0u);

    if (queue >= kThreadQueueArrayAddr &&
        queue < (kThreadQueueArrayAddr + kThreadQueueArrayBytes) &&
        ((queue - kThreadQueueArrayAddr) % 8u) == 0u &&
        Memory::Read32(queue) == 0u) {
        const std::uint32_t priority = (queue - kThreadQueueArrayAddr) / 8u;
        const std::uint32_t pending = Memory::Read32(kSchedulerPendingFlagAddr);
        Memory::Write32(kSchedulerPendingFlagAddr, pending & ~(1u << (31u - priority)));
    }
}

void TryInvokeSwitchCallback(std::uint32_t oldContext,
                             std::uint32_t newContext,
                             CpuContext* cpu) {
    if (!cpu || !Mapped32(kSwitchThreadCallbackPtrAddr)) {
        return;
    }

    const std::uint32_t callback = Memory::Read32(kSwitchThreadCallbackPtrAddr);
    if (callback == 0u) {
        return;
    }

    cpu->gpr[3] = oldContext;
    cpu->gpr[4] = newContext;
    // Pinned WiiCompiled skips an unregistered callback instead of making the
    // scheduler fail. The Switch dynamic dispatcher gives us the same test:
    // execute it when present, otherwise continue with the scheduler state.
    (void)mkw_switch_try_dispatch_indirect(callback, cpu);
}

} // namespace

extern "C" void mkw_switch_hle_select_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t forceSwitch = cpu->gpr[3];

    try {
        if (!Mapped32(kSchedulerIdleFlagAddr) ||
            !Mapped32(kOSCurrentContextAddr) ||
            !Mapped32(kOSRunningContextAddr) ||
            !Mapped32(kSchedulerPendingFlagAddr) ||
            !Mapped32(kSchedulerReschedCounterAddr)) {
            AbortSelectBoundary("SELECTTHREAD_BAD_GLOBAL_STATE", cpu);
        }

        // Pinned WiiCompiled calls its host sleep-timer pump here. The Switch
        // fast-track does not own a separate host timer queue yet; no timer is
        // fabricated. If the scheduler reaches its idle polling path below,
        // keep that missing timing model as an explicit hardware blocker.
        if (Memory::Read32(kSchedulerIdleFlagAddr) >= 1u) {
            cpu->gpr[3] = 0u;
            return;
        }

        const std::uint32_t currentContext = Memory::Read32(kOSCurrentContextAddr);
        const std::uint32_t runningContext = Memory::Read32(kOSRunningContextAddr);

        if (currentContext == 0u && runningContext == 0u) {
            cpu->gpr[3] = 0u;
            return;
        }
        if (currentContext != runningContext) {
            cpu->gpr[3] = 0u;
            return;
        }

        if (runningContext != 0u) {
            if (!Mapped16(runningContext + kThreadStateOffset) ||
                !Mapped32(runningContext + kThreadPriorityOffset) ||
                !Mapped16(runningContext + kContextModeFlagsOffset)) {
                AbortSelectBoundary("SELECTTHREAD_BAD_RUNNING_CONTEXT", cpu);
            }

            const std::uint16_t threadState =
                Memory::Read16(runningContext + kThreadStateOffset);
            if (threadState == kThreadStateRunning) {
                const std::uint32_t pending = Memory::Read32(kSchedulerPendingFlagAddr);
                if (forceSwitch == 0u) {
                    const std::uint32_t pendingPriority =
                        pending == 0u ? 32u : PPC_Cntlzw(pending);
                    const std::int32_t runningPriority =
                        static_cast<std::int32_t>(
                            Memory::Read32(runningContext + kThreadPriorityOffset));
                    if (runningPriority <= static_cast<std::int32_t>(pendingPriority)) {
                        cpu->gpr[3] = 0u;
                        return;
                    }
                }

                const std::int32_t priority =
                    static_cast<std::int32_t>(
                        Memory::Read32(runningContext + kThreadPriorityOffset));
                if (priority < 0 || priority > 31) {
                    AbortSelectBoundary("SELECTTHREAD_BAD_RUNNING_PRIORITY", cpu);
                }

                Memory::Write16(runningContext + kThreadStateOffset, kThreadStateReady);
                LinkThreadOnRunQueue(runningContext, priority);
                MarkRunQueuePending(priority);
            }

            const std::uint16_t modeFlags =
                Memory::Read16(runningContext + kContextModeFlagsOffset);
            if ((modeFlags & 0x0002u) == 0u) {
                // Preserve the pin's OSSaveContext dependency rather than
                // inventing a saved PPC context. If the translated helper is
                // not in the active product graph, the generic indirect
                // dispatcher records that exact next boundary.
                cpu->gpr[3] = runningContext;
                InvokeIndirectCpu(0x801A1ED8u, cpu);
                if (cpu->gpr[3] != 0u) {
                    cpu->gpr[3] = 0u;
                    return;
                }
            }
        }

        std::uint32_t pendingMask = Memory::Read32(kSchedulerPendingFlagAddr);
        if (pendingMask == 0u) {
            // The pin enters a host-driven idle loop that pumps sleep timers,
            // audio, alarms and VI retraces. None of those independent host
            // pumps is proven on Horizon yet, so do not spin or fake a wakeup.
            AbortSelectBoundary("SELECTTHREAD_IDLE_POLL", cpu);
        }

        Memory::Write32(kSchedulerReschedCounterAddr, 0u);

        std::uint32_t priorityLevel = 0u;
        std::uint32_t queueEntry = 0u;
        std::uint32_t nextThread = 0u;
        while (pendingMask != 0u) {
            priorityLevel = PPC_Cntlzw(pendingMask);
            if (priorityLevel > 31u) {
                AbortSelectBoundary("SELECTTHREAD_BAD_PENDING_MASK", cpu);
            }

            queueEntry = kThreadQueueArrayAddr + priorityLevel * 8u;
            nextThread = Memory::Read32(queueEntry);
            if (nextThread == 0u) {
                pendingMask &= ~(1u << (31u - priorityLevel));
                Memory::Write32(kSchedulerPendingFlagAddr, pendingMask);
                continue;
            }

            const std::uint16_t state = Memory::Read16(nextThread + kThreadStateOffset);
            if (state == 0u || state == kThreadStateMoribund) {
                RemoveThreadFromQueue(nextThread);
                pendingMask = Memory::Read32(kSchedulerPendingFlagAddr);
                continue;
            }
            break;
        }

        if (nextThread == 0u) {
            cpu->gpr[3] = 0u;
            return;
        }

        const std::uint32_t threadNext = PopThreadQueueHead(queueEntry, nextThread);
        if (threadNext == 0u) {
            const std::uint32_t mask = ~(1u << (31u - priorityLevel));
            Memory::Write32(kSchedulerPendingFlagAddr, pendingMask & mask);
        }

        Memory::Write32(nextThread + kThreadQueueOffset, 0u);
        Memory::Write16(nextThread + kThreadStateOffset, kThreadStateRunning);

        TryInvokeSwitchCallback(runningContext, nextThread, cpu);
        Memory::Write32(kOSRunningContextAddr, nextThread);

        cpu->gpr[3] = nextThread;
        InvokeDirectCpu<0x801A1E70u>(cpu); // OSSetCurrentContext

        // The latest hardware trace proves that an OSContext can resume at an
        // instruction inside an already translated C++ function (0x80238A78 in
        // EGG::ProcessMeter::__ct). Pinned WiiCompiled solves that class of
        // continuation by preserving each guest OSThread's native host stack.
        // Adopt the same path when OSCreateThread gave the target a HostContext.
        if (mkw::switch_guest_fiber::available() &&
            mkw::switch_guest_fiber::has(nextThread)) {
            if (mkw::switch_guest_fiber::current_thread() == 0u && runningContext != 0u) {
                if (!mkw::switch_guest_fiber::has(runningContext) &&
                    !mkw::switch_guest_fiber::register_current(runningContext, cpu)) {
                    AbortSelectBoundary("SELECTTHREAD_REGISTER_MAIN_FIBER", cpu);
                }
            }

            if (!mkw::switch_guest_fiber::switch_to(nextThread, cpu)) {
                AbortSelectBoundary("SELECTTHREAD_GUEST_FIBER_SWITCH", cpu);
            }
            cpu->gpr[3] = 0u;
            return;
        }

        // Keep the already hardware-proven non-fiber fallback for contexts that
        // were not created through OSCreateThread. Its SRR0 dispatch remains a
        // durable blocker rather than manufacturing a translated continuation.
        cpu->gpr[3] = nextThread;
        InvokeDirectCpu<0x801A1F58u>(cpu);
    } catch (...) {
        AbortSelectBoundary("SELECTTHREAD_GUEST_MEMORY", cpu);
    }
}

#endif
