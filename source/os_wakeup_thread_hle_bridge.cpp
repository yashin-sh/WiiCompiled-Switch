#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t kDefaultThreadContextAddr = 0x80347498u;
constexpr std::uint32_t kThreadQueueArrayAddr = 0x803477B0u;
constexpr std::uint32_t kSchedulerReschedCounterAddr = 0x8038691Cu;
constexpr std::uint32_t kSchedulerPendingFlagAddr = 0x80386920u;

constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadSuspendOffset = 0x2CCu;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;
constexpr std::uint32_t kThreadNextOffset = 0x2E0u;
constexpr std::uint32_t kThreadPrevOffset = 0x2E4u;

constexpr std::uint16_t kThreadStateReady = 1u;
constexpr std::uint16_t kThreadStateMoribund = 8u;
constexpr int kMaxWake = 256;

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

bool ValidateThread(std::uint32_t thread) noexcept {
    return Mapped16(thread + kThreadStateOffset) &&
           Mapped32(thread + kThreadSuspendOffset) &&
           Mapped32(thread + kThreadPriorityOffset) &&
           Mapped32(thread + kThreadQueueOffset) &&
           Mapped32(thread + kThreadNextOffset) &&
           Mapped32(thread + kThreadPrevOffset);
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

void LinkThreadOnRunQueue(std::uint32_t thread, std::int32_t priority) {
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

} // namespace

extern "C" void mkw_switch_hle_os_wakeup_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t queueAddr = cpu->gpr[3];
    if (queueAddr == 0u) {
        return;
    }

    mkw_switch_hle_os_disable_interrupts(cpu);
    const std::uint32_t irqState = cpu->gpr[3];
    bool resched = false;

    try {
        if (!Mapped32(queueAddr) || !Mapped32(queueAddr + 4u) ||
            !Mapped32(kSchedulerPendingFlagAddr) ||
            !Mapped32(kSchedulerReschedCounterAddr)) {
            RestoreInterrupts(cpu, irqState);
            return;
        }

        int woke = 0;
        while (woke < kMaxWake) {
            const std::uint32_t thread = Memory::Read32(queueAddr);
            if (thread == 0u) {
                break;
            }
            if (!ValidateThread(thread)) {
                RestoreInterrupts(cpu, irqState);
                return;
            }

            PopThreadQueueHead(queueAddr, thread);

            const std::uint16_t state = Memory::Read16(thread + kThreadStateOffset);
            if (state == 0u || state == kThreadStateMoribund) {
                Memory::Write32(thread + kThreadQueueOffset, 0u);
                Memory::Write32(thread + kThreadNextOffset, 0u);
                Memory::Write32(thread + kThreadPrevOffset, 0u);
                ++woke;
                continue;
            }

            Memory::Write16(thread + kThreadStateOffset, kThreadStateReady);

            const std::int32_t suspend =
                static_cast<std::int32_t>(Memory::Read32(thread + kThreadSuspendOffset));
            if (suspend < 1) {
                std::int32_t priority =
                    static_cast<std::int32_t>(Memory::Read32(thread + kThreadPriorityOffset));
                if (priority < 0) {
                    priority = 0;
                } else if (priority > 31) {
                    priority = 31;
                }

                LinkThreadOnRunQueue(thread, priority);

                // Match the pin's guest-fiber bookkeeping. The default thread may
                // already be represented by the scheduler HostContext; registration
                // is best-effort because register_current only succeeds on that host.
                if (mkw::switch_guest_fiber::available()) {
                    if (thread == kDefaultThreadContextAddr &&
                        !mkw::switch_guest_fiber::has(thread)) {
                        (void)mkw::switch_guest_fiber::register_current(thread, cpu);
                    }
                    mkw::switch_guest_fiber::resume(thread);
                }

                MarkRunQueuePending(priority);
                resched = true;
            }

            ++woke;
        }

        // Pinned WiiCompiled suppresses immediate SelectThread recursion while
        // AdvanceRetrace is delivering VI callbacks. The 2026-09-23 hardware
        // trace proves AsyncDisplay's sync queue is woken from PostRetrace while
        // SelectThread itself is in the idle VI pump, so mirror that guard here.
        // The outer SelectThread observes the pending bit and performs the switch
        // after the retrace callback has unwound.
        if (resched && !mkw_switch_hle_vi_retrace_advancing()) {
            cpu->gpr[3] = 0u;
            InvokeDirectCpu<0x801A9C08u>(cpu);
        }
    } catch (...) {
        // Keep malformed guest scheduler state contained at this native boundary.
    }

    RestoreInterrupts(cpu, irqState);
}

#endif
