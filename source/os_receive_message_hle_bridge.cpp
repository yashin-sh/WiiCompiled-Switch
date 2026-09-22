#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdio>

namespace {

constexpr std::uint32_t kMsgQueueSendOffset = 0x00u;
constexpr std::uint32_t kMsgQueueRecvOffset = 0x08u;
constexpr std::uint32_t kMsgQueueArrayOffset = 0x10u;
constexpr std::uint32_t kMsgQueueCountOffset = 0x14u;
constexpr std::uint32_t kMsgQueueFirstOffset = 0x18u;
constexpr std::uint32_t kMsgQueueUsedOffset = 0x1Cu;

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
constexpr const char* kOsMessageEventsPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-os-message-events.txt";
std::uint32_t gOsMessageEventSequence = 0u;
constexpr const char* kOsReceiveMessageFrontierPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-os-receive-message-frontier.txt";
std::uint32_t gOsReceiveMessageFrontierSequence = 0u;

std::uint32_t ReadQueue32OrZero(std::uint32_t address) noexcept {
    try {
        if (!Memory::Contains(address, 4u)) {
            return 0u;
        }
        return Memory::Read32(address);
    } catch (...) {
        return 0u;
    }
}

void WriteReceiveMessageFrontier(
    const char* phase,
    std::uint32_t queuePtr,
    std::uint32_t outMsgPtr,
    std::uint32_t msg,
    CpuContext* cpu) noexcept {
    const char* mode = gOsReceiveMessageFrontierSequence == 0u ? "w" : "a";
    FILE* out = std::fopen(kOsReceiveMessageFrontierPath, mode);
    if (!out) {
        return;
    }

    ++gOsReceiveMessageFrontierSequence;
    const std::uint32_t arrayPtr = ReadQueue32OrZero(queuePtr + kMsgQueueArrayOffset);
    const std::uint32_t count = ReadQueue32OrZero(queuePtr + kMsgQueueCountOffset);
    const std::uint32_t first = ReadQueue32OrZero(queuePtr + kMsgQueueFirstOffset);
    const std::uint32_t used = ReadQueue32OrZero(queuePtr + kMsgQueueUsedOffset);
    const std::uint32_t sendHead = ReadQueue32OrZero(queuePtr + 0x00u);
    const std::uint32_t sendTail = ReadQueue32OrZero(queuePtr + 0x04u);
    const std::uint32_t recvHead = ReadQueue32OrZero(queuePtr + 0x08u);
    const std::uint32_t recvTail = ReadQueue32OrZero(queuePtr + 0x0Cu);
    const std::uint32_t outValue =
        outMsgPtr != 0u ? ReadQueue32OrZero(outMsgPtr) : 0u;
    const std::uint32_t currentThread = ReadQueue32OrZero(0x800000D4u);
    const std::uint32_t runningThread = ReadQueue32OrZero(0x800000E4u);

    std::fprintf(
        out,
        "event=%u phase=%s queue=0x%08x out=0x%08x msg=0x%08x out_value=0x%08x "
        "array=0x%08x count=%u first=%u used=%u send_head=0x%08x send_tail=0x%08x "
        "recv_head=0x%08x recv_tail=0x%08x os_current=0x%08x os_running=0x%08x "
        "pc=0x%08x r1=0x%08x r3=0x%08x r4=0x%08x r5=0x%08x lr=0x%08x\n",
        gOsReceiveMessageFrontierSequence,
        phase ? phase : "<null>",
        queuePtr,
        outMsgPtr,
        msg,
        outValue,
        arrayPtr,
        count,
        first,
        used,
        sendHead,
        sendTail,
        recvHead,
        recvTail,
        currentThread,
        runningThread,
        cpu ? cpu->pc : 0u,
        cpu ? cpu->gpr[1] : 0u,
        cpu ? cpu->gpr[3] : 0u,
        cpu ? cpu->gpr[4] : 0u,
        cpu ? cpu->gpr[5] : 0u,
        cpu ? cpu->lr : 0u);
    std::fclose(out);
}

void WriteSendMessageEvent(
    std::uint32_t queuePtr,
    std::uint32_t msg,
    CpuContext* cpu) noexcept {
    const char* mode = gOsMessageEventSequence == 0u ? "w" : "a";
    FILE* out = std::fopen(kOsMessageEventsPath, mode);
    if (!out) {
        return;
    }

    ++gOsMessageEventSequence;
    const std::uint32_t arrayPtr = ReadQueue32OrZero(queuePtr + kMsgQueueArrayOffset);
    const std::uint32_t count = ReadQueue32OrZero(queuePtr + kMsgQueueCountOffset);
    const std::uint32_t first = ReadQueue32OrZero(queuePtr + kMsgQueueFirstOffset);
    const std::uint32_t used = ReadQueue32OrZero(queuePtr + kMsgQueueUsedOffset);

    std::fprintf(
        out,
        "event=%u kind=send queue=0x%08x msg=0x%08x array=0x%08x count=%u first=%u used_before=%u "
        "pc=0x%08x r1=0x%08x r3=0x%08x r4=0x%08x r5=0x%08x lr=0x%08x\n",
        gOsMessageEventSequence,
        queuePtr,
        msg,
        arrayPtr,
        count,
        first,
        used,
        cpu ? cpu->pc : 0u,
        cpu ? cpu->gpr[1] : 0u,
        cpu ? cpu->gpr[3] : 0u,
        cpu ? cpu->gpr[4] : 0u,
        cpu ? cpu->gpr[5] : 0u,
        cpu ? cpu->lr : 0u);
    std::fclose(out);
}
#else
void WriteReceiveMessageFrontier(
    const char*, std::uint32_t, std::uint32_t, std::uint32_t, CpuContext*) noexcept {}
void WriteSendMessageEvent(std::uint32_t, std::uint32_t, CpuContext*) noexcept {}
#endif

bool QueueIsFull(std::uint32_t queuePtr) {
    const std::uint32_t used = Memory::Read32(queuePtr + kMsgQueueUsedOffset);
    const std::uint32_t count = Memory::Read32(queuePtr + kMsgQueueCountOffset);
    return count != 0u && used >= count;
}

void EnqueueMessage(std::uint32_t queuePtr, std::uint32_t msg) {
    const std::uint32_t arrayPtr = Memory::Read32(queuePtr + kMsgQueueArrayOffset);
    const std::uint32_t count = Memory::Read32(queuePtr + kMsgQueueCountOffset);
    const std::uint32_t first = Memory::Read32(queuePtr + kMsgQueueFirstOffset);
    const std::uint32_t used = Memory::Read32(queuePtr + kMsgQueueUsedOffset);

    if (count == 0u) {
        return;
    }

    const std::uint32_t index = (first + used) % count;
    Memory::Write32(arrayPtr + index * 4u, msg);
    Memory::Write32(queuePtr + kMsgQueueUsedOffset, used + 1u);
}

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

extern "C" void mkw_switch_hle_os_send_message(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t queuePtr = cpu->gpr[3];
    const std::uint32_t msg = cpu->gpr[4];
    const bool block = (cpu->gpr[5] & 1u) != 0u;

    if (queuePtr == 0u) {
        cpu->gpr[3] = 0u;
        return;
    }

    WriteSendMessageEvent(queuePtr, msg, cpu);

    // Match pinned WiiCompiled's MsgQueueOp path used by OSSendMessage:
    // disable interrupts once, append when space exists and wake receivers.
    // A full blocking send parks on the embedded send OSThreadQueue and retries.
    mkw_switch_hle_os_disable_interrupts(cpu);
    const bool previousInterruptState = cpu->gpr[3] != 0u;

    while (true) {
        try {
            if (!QueueIsFull(queuePtr)) {
                EnqueueMessage(queuePtr, msg);

                cpu->gpr[3] = queuePtr + kMsgQueueRecvOffset;
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

        cpu->gpr[3] = queuePtr + kMsgQueueSendOffset;
        InvokeDirectCpu<0x801AA9B8u>(cpu);
    }
}

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
                WriteReceiveMessageFrontier(
                    "ready-before-dequeue", queuePtr, outMsgPtr, 0u, cpu);
                const std::uint32_t msg = DequeueMessage(queuePtr);
                WriteReceiveMessageFrontier(
                    "after-dequeue", queuePtr, outMsgPtr, msg, cpu);
                if (outMsgPtr != 0u) {
                    Memory::Write32(outMsgPtr, msg);
                }
                WriteReceiveMessageFrontier(
                    "after-output-write", queuePtr, outMsgPtr, msg, cpu);

                // Pinned OSReceiveMessage wakes senders after making one slot
                // available. Keep OSWakeupThread as the next explicit native
                // boundary until hardware proves that scheduler path is needed.
                cpu->gpr[3] = queuePtr + kMsgQueueSendOffset;
                InvokeDirectCpu<0x801AAAA4u>(cpu);
                WriteReceiveMessageFrontier(
                    "after-wakeup-senders", queuePtr, outMsgPtr, msg, cpu);

                RestoreInterruptState(cpu, previousInterruptState);
                WriteReceiveMessageFrontier(
                    "after-restore-interrupts", queuePtr, outMsgPtr, msg, cpu);
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
        WriteReceiveMessageFrontier(
            "before-block-sleep", queuePtr, outMsgPtr, 0u, cpu);
        cpu->gpr[3] = queuePtr + kMsgQueueRecvOffset;
        InvokeDirectCpu<0x801AA9B8u>(cpu);
        WriteReceiveMessageFrontier(
            "after-block-sleep", queuePtr, outMsgPtr, 0u, cpu);
    }
}

#endif
