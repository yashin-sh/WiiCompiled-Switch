#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_os_hle_traits.hpp"

#include <cstdint>
#include <cstdio>

namespace {

constexpr std::uint32_t kTaskMessageQueueOffset = 0x0Cu;
constexpr std::uint32_t kTaskCurrentJobOffset = 0x48u;
constexpr std::uint32_t kTaskDoneQueueOffset = 0x54u;

constexpr std::uint32_t kJobCallbackOffset = 0x00u;
constexpr std::uint32_t kJobArgOffset = 0x04u;
constexpr std::uint32_t kJobTokenOffset = 0x08u;
constexpr std::uint32_t kJobUnknown3Offset = 0x0Cu;
constexpr std::uint32_t kJobUnknown4Offset = 0x10u;
constexpr std::uint32_t kJobOnDoneOffset = 0x14u;
constexpr std::uint32_t kJobSize = 0x18u;

constexpr const char* kTaskThreadDispatchPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-task-thread-last-dispatch.txt";

std::uint32_t Read32OrZero(std::uint32_t address) noexcept {
    try {
        if (!Memory::Contains(address, 4u)) {
            return 0u;
        }
        return Memory::Read32(address);
    } catch (...) {
        return 0u;
    }
}

void WriteTaskThreadDispatchFrontier(
    const char* kind,
    std::uint32_t taskThread,
    std::uint32_t stackPointer,
    std::uint32_t outMsgPtr,
    std::uint32_t job,
    std::uint32_t callback,
    std::uint32_t arg,
    std::uint32_t token,
    std::uint32_t onDone,
    std::uint32_t target,
    CpuContext* cpu) noexcept {
#if defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION
    FILE* out = std::fopen(kTaskThreadDispatchPath, "w");
    if (!out) {
        return;
    }

    const std::uint32_t queuePtr = taskThread + kTaskMessageQueueOffset;
    const std::uint32_t queueArray = Read32OrZero(queuePtr + 0x10u);
    const std::uint32_t queueCount = Read32OrZero(queuePtr + 0x14u);
    const std::uint32_t queueFirst = Read32OrZero(queuePtr + 0x18u);
    const std::uint32_t queueUsed = Read32OrZero(queuePtr + 0x1Cu);
    const std::uint32_t memberMesgBuffer = Read32OrZero(taskThread + 0x2Cu);
    const std::uint32_t memberMesgCount = Read32OrZero(taskThread + 0x30u);
    const std::uint32_t stackMemory = Read32OrZero(taskThread + 0x34u);
    const std::uint32_t stackSize = Read32OrZero(taskThread + 0x38u);
    const std::uint32_t jobs = Read32OrZero(taskThread + 0x4Cu);
    const std::uint32_t jobCount = Read32OrZero(taskThread + 0x50u);

    std::fprintf(
        out,
        "WiiCompiled-Switch TaskThread last indirect dispatch\n"
        "===================================================\n"
        "kind                  : %s\n"
        "task thread           : 0x%08x\n"
        "stack pointer         : 0x%08x\n"
        "out message pointer   : 0x%08x\n"
        "job                   : 0x%08x\n"
        "callback              : 0x%08x\n"
        "arg                   : 0x%08x\n"
        "token                 : 0x%08x\n"
        "onDone                : 0x%08x\n"
        "queue ptr             : 0x%08x\n"
        "queue array           : 0x%08x\n"
        "queue count           : 0x%08x\n"
        "queue first           : 0x%08x\n"
        "queue used            : 0x%08x\n"
        "member mesg buffer    : 0x%08x\n"
        "member mesg count     : 0x%08x\n"
        "stack memory          : 0x%08x\n"
        "stack size            : 0x%08x\n"
        "jobs                  : 0x%08x\n"
        "job count             : 0x%08x\n"
        "dispatch target       : 0x%08x\n"
        "cpu r1                : 0x%08x\n"
        "cpu r3                : 0x%08x\n"
        "cpu r4                : 0x%08x\n"
        "cpu r5                : 0x%08x\n",
        kind ? kind : "<null>",
        taskThread,
        stackPointer,
        outMsgPtr,
        job,
        callback,
        arg,
        token,
        onDone,
        queuePtr,
        queueArray,
        queueCount,
        queueFirst,
        queueUsed,
        memberMesgBuffer,
        memberMesgCount,
        stackMemory,
        stackSize,
        jobs,
        jobCount,
        target,
        cpu ? cpu->gpr[1] : 0u,
        cpu ? cpu->gpr[3] : 0u,
        cpu ? cpu->gpr[4] : 0u,
        cpu ? cpu->gpr[5] : 0u);
    std::fclose(out);
#else
    (void)kTaskThreadDispatchPath;
    (void)kind;
    (void)taskThread;
    (void)stackPointer;
    (void)outMsgPtr;
    (void)job;
    (void)callback;
    (void)arg;
    (void)token;
    (void)onDone;
    (void)target;
    (void)cpu;
#endif
}

bool ShouldRetryThpPrepareAfterClose(
    std::uint32_t callback,
    std::uint32_t arg,
    std::uint32_t result) {
    if (callback != 0x80529D68u || result != 0u) {
        return false;
    }
    if (arg == 0u || !Memory::Contains(arg + 0xACu, 4u)) {
        return false;
    }
    if (Memory::Read32(arg + 0xACu) != 0u) {
        return false;
    }
    if (!Memory::Contains(0x809BECF0u, 4u) || !Memory::Contains(0x809BEBA0u, 4u)) {
        return false;
    }
    return Memory::Read32(0x809BECF0u) != 0u && Memory::Read32(0x809BEBA0u) != 0u;
}

std::uint32_t RunMovieManagerPrepareAsync(std::uint32_t manager, CpuContext* cpu) {
    if (!cpu || manager == 0u || !Memory::Contains(manager + 0xACu, 4u)) {
        return 0u;
    }

    if (Memory::Read32(manager + 0xACu) != 0u) {
        return cpu->gpr[3];
    }

    auto callThpPlayerOpen = [&]() -> std::uint32_t {
        CpuContextScope scope(cpu);
        cpu->gpr[3] = manager + 0x28u;
        cpu->gpr[4] = 0u;
        InvokeIndirectCpu(0x80550CC0u, cpu);
        return cpu->gpr[3];
    };

    std::uint32_t openResult = callThpPlayerOpen();
    if (openResult == 0u && ShouldRetryThpPrepareAfterClose(0x80529D68u, manager, 0u)) {
        {
            CpuContextScope scope(cpu);
            InvokeIndirectCpu(0x80551658u, cpu);
            InvokeIndirectCpu(0x80550F48u, cpu);
        }
        if (Memory::Contains(0x809BEBA0u, 8u) && Memory::Read32(0x809BEBA0u) != 0u) {
            Memory::Write32(0x809BEBA0u, 0u);
            Memory::Write8(0x809BEBA4u, 0u);
            Memory::Write8(0x809BEBA5u, 0u);
            Memory::Write8(0x809BEBA6u, 0u);
            Memory::Write8(0x809BEBA7u, 0u);
        }
        openResult = callThpPlayerOpen();
    }

    if (openResult == 0u) {
        cpu->gpr[3] = 0u;
        return 0u;
    }

    {
        CpuContextScope scope(cpu);
        cpu->gpr[3] = manager + 0x10u;
        InvokeIndirectCpu(0x80551D38u, cpu);
        InvokeIndirectCpu(0x80550F9Cu, cpu);
    }
    const std::uint32_t needMemory = cpu->gpr[3];
    Memory::Write32(manager + 0x20u, needMemory);
    Memory::Write32(manager + 0x24u, 0u);

    {
        CpuContextScope scope(cpu);
        cpu->gpr[3] = Memory::Read32(manager + 0x1Cu);
        InvokeIndirectCpu(0x80551054u, cpu);
    }

    const std::uint32_t mode = Memory::Read32(manager + 0xA8u);
    const std::uint32_t audioSystem = Memory::Read32(0x8088FDB8u + (mode << 2u));
    std::uint32_t prepareResult = 0u;
    {
        CpuContextScope scope(cpu);
        cpu->gpr[3] = Memory::Read32(manager + 0x24u);
        cpu->gpr[4] = audioSystem;
        cpu->gpr[5] = 0u;
        InvokeIndirectCpu(0x80551378u, cpu);
        prepareResult = cpu->gpr[3];
    }
    if (prepareResult != 0u) {
        Memory::Write32(manager + 0xACu, 1u);
    }
    cpu->gpr[3] = prepareResult;
    return prepareResult;
}

void ClearTaskJob(std::uint32_t taskThread, std::uint32_t job) {
    if (job == 0u || !Memory::Contains(job, kJobSize)) {
        Memory::Write32(taskThread + kTaskCurrentJobOffset, 0u);
        return;
    }

    Memory::Write32(job + kJobCallbackOffset, 0u);
    Memory::Write32(taskThread + kTaskCurrentJobOffset, 0u);
    Memory::Write32(job + kJobCallbackOffset, 0u);
    Memory::Write32(job + kJobUnknown3Offset, 0u);
    Memory::Write32(job + kJobUnknown4Offset, 0u);
    Memory::Write32(job + kJobOnDoneOffset, 0u);
}

} // namespace

extern "C" void mkw_switch_hle_task_thread_run(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    CpuContext* cpu = ctx;
    const std::uint32_t taskThread = cpu->gpr[3];
    if (!Memory::Contains(taskThread + kTaskMessageQueueOffset, 0x4Cu)) {
        return;
    }

    // Exact pinned TaskThread::run worker prelude.
    cpu->gqr[2] = 0x00040004u;
    cpu->gqr[3] = 0x00050005u;
    cpu->gqr[4] = 0x00060006u;
    cpu->gqr[5] = 0x00070007u;

    while (true) {
        const std::uint32_t stackPointer = cpu->gpr[1];
        const std::uint32_t outMsgPtr = stackPointer >= 0x20u ? stackPointer - 0x20u : 0u;
        if (outMsgPtr == 0u || !Memory::Contains(outMsgPtr, 4u)) {
            return;
        }

        {
            CpuContextScope scope(cpu);
            cpu->gpr[3] = taskThread + kTaskMessageQueueOffset;
            cpu->gpr[4] = outMsgPtr;
            cpu->gpr[5] = 1u;
            InvokeDirectCpu<0x801A7424u>(cpu);
        }

        const std::uint32_t job = Memory::Read32(outMsgPtr);
        Memory::Write32(taskThread + kTaskCurrentJobOffset, job);

        if (job != 0u && Memory::Contains(job, kJobSize)) {
            const std::uint32_t callback = Memory::Read32(job + kJobCallbackOffset);
            const std::uint32_t arg = Memory::Read32(job + kJobArgOffset);
            const std::uint32_t token = Memory::Read32(job + kJobTokenOffset);
            const std::uint32_t onDone = Memory::Read32(job + kJobOnDoneOffset);

            if (callback != 0u) {
                CpuContextScope scope(cpu);
                if (callback == 0x80529D68u) {
                    RunMovieManagerPrepareAsync(arg, cpu);
                } else {
                    cpu->gpr[3] = arg;
                    WriteTaskThreadDispatchFrontier(
                        "callback",
                        taskThread,
                        stackPointer,
                        outMsgPtr,
                        job,
                        callback,
                        arg,
                        token,
                        onDone,
                        callback,
                        cpu);
                    InvokeIndirectCpu(callback, cpu);
                }
            }

            const std::uint32_t currentJob =
                Memory::Read32(taskThread + kTaskCurrentJobOffset);
            if (currentJob != 0u && onDone != 0u) {
                CpuContextScope scope(cpu);
                cpu->gpr[3] = Memory::Read32(currentJob + kJobArgOffset);
                WriteTaskThreadDispatchFrontier(
                    "onDone",
                    taskThread,
                    stackPointer,
                    outMsgPtr,
                    currentJob,
                    callback,
                    cpu->gpr[3],
                    token,
                    onDone,
                    onDone,
                    cpu);
                InvokeIndirectCpu(onDone, cpu);
            }

            if (Memory::Contains(taskThread + kTaskDoneQueueOffset, 4u)) {
                const std::uint32_t doneQueue =
                    Memory::Read32(taskThread + kTaskDoneQueueOffset);
                if (doneQueue != 0u) {
                    CpuContextScope scope(cpu);
                    cpu->gpr[3] = doneQueue;
                    cpu->gpr[4] = token;
                    cpu->gpr[5] = 0u;
                    InvokeDirectCpu<0x801A735Cu>(cpu);
                }
            }
        }

        ClearTaskJob(taskThread, job);
    }
}

#endif
