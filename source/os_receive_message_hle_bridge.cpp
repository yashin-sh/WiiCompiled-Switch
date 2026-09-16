#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>

namespace {

constexpr std::uint32_t kMsgQueueSendOffset = 0x00u;
constexpr std::uint32_t kMsgQueueRecvOffset = 0x08u;
constexpr std::uint32_t kMsgQueueArrayOffset = 0x10u;
constexpr std::uint32_t kMsgQueueCountOffset = 0x14u;
constexpr std::uint32_t kMsgQueueFirstOffset = 0x18u;
constexpr std::uint32_t kMsgQueueUsedOffset = 0x1Cu;

std::uint32_t DequeueMessage(std::uint32_t queuePtr) {
    const std::uint32_t arrayPtr = Memory::Read32(queuePtr + kMsgQueueArrayOffset);
    const std::uint32_t count = Memory::Read32(queuePtr + kMsgQueueCountOffset);
    std::uint32_t first = Memory::Read32(queuePtr + kMsgQueueFirstOffset);
    const std::uint32_t used = Memory::Read32(queuePtr + kMsgQueueUsedOffset);

    if (count == 0u || used == 0u) {
        return 0u;
    }

    const std::uint32_t msg = Memory::Read32(arrayPtr + first * 4u);
    first = (first + 1u) % count;
    Memory::Write32(queuePtr + kMsgQueueFirstOffset, first);
    Memory::Write32(queuePtr + kMsgQueueUsedOffset, used - 1u);
    return msg;
}

void RestoreInterruptState(CpuContext* cpu, bool enabled) noexcept {
    cpu->gpr[3] = enabled ? 1u : 0u;
    mkw_switch_hle_os_restore_interrupts(cpu);
}

} // namespace

extern "C" void mkw_switch_hle_os_receive_message(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t queuePtr = cpu->gpr[3];
    const std::uint32_t outMsgPtr = cpu->gpr[4];
    const bool block = (cpu->gpr[5] & 1u) != 0u;

    if (queuePtr == 0u) {
        cpu->gpr[3] = 0u;
        return;
    }

    // Match pinned WiiCompiled's MsgQueueOp entry: disable interrupts once and
    // keep them disabled across blocking sleeps until this receive completes.
    mkw_switch_hle_os_disable_interrupts(cpu);
    const bool previousInterruptState = cpu->gpr[3] != 0u;

    while (true) {
        try {
            const std::uint32_t used = Memory::Read32(queuePtr + kMsgQueueUsedOffset);
            if (used != 0u) {
                const std::uint32_t msg = DequeueMessage(queuePtr);
                if (outMsgPtr != 0u) {
                    Memory::Write32(outMsgPtr, msg);
                }

                // Pinned OSReceiveMessage wakes senders after making one slot
                // available. Keep OSWakeupThread as the next explicit native
                // boundary until hardware proves that scheduler path is needed.
                cpu->gpr[3] = queuePtr + kMsgQueueSendOffset;
                InvokeDirectCpu<0x801AAAA4u>(cpu);

                RestoreInterruptState(cpu, previousInterruptState);
                cpu->gpr[3] = 1u;
                return;
            }
        } catch (...) {
            RestoreInterruptState(cpu, previousInterruptState);
            cpu->gpr[3] = 0u;
            return;
        }

        if (!block) {
            RestoreInterruptState(cpu, previousInterruptState);
            cpu->gpr[3] = 0u;
            return;
        }

        // Empty blocking receive: pinned WiiCompiled parks the current thread
        // on the receive wait queue, then rechecks after it is woken. Do not
        // fabricate OSSleepThread semantics before hardware reaches that target.
        cpu->gpr[3] = queuePtr + kMsgQueueRecvOffset;
        InvokeDirectCpu<0x801AA9B8u>(cpu);
    }
}

#endif
