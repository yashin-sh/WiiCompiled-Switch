#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "fast_track_thread_diagnostics.hpp"
#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t kThreadSize = 0x318u;
constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadAttrOffset = 0x2CAu;
constexpr std::uint32_t kThreadSuspendOffset = 0x2CCu;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadBasePriorityOffset = 0x2D4u;
constexpr std::uint32_t kThreadExitValueOffset = 0x2D8u;
constexpr std::uint32_t kThreadJoinQueueOffset = 0x2E8u;
constexpr std::uint32_t kThreadMutexOffset = 0x2F0u;
constexpr std::uint32_t kThreadMutexQueueOffset = 0x2F4u;
constexpr std::uint32_t kThreadMutexTailOffset = 0x2F8u;
constexpr std::uint32_t kThreadListNextOffset = 0x2FCu;
constexpr std::uint32_t kThreadListPrevOffset = 0x300u;

constexpr std::uint32_t kThreadStackTopOffset = 0x304u;
constexpr std::uint32_t kThreadStackBottomOffset = 0x308u;
constexpr std::uint32_t kThreadSpecific0Offset = 0x30Cu;
constexpr std::uint32_t kThreadSpecific1Offset = 0x310u;
constexpr std::uint32_t kThreadSpecific2Offset = 0x314u;

constexpr std::uint32_t kThreadListHeadAddr = 0x800000DCu;
constexpr std::uint32_t kThreadListTailAddr = 0x800000E0u;
constexpr std::uint32_t kCurrentFpuContextAddr = 0x800000D8u;
constexpr std::uint32_t kSchedulerInitFlagAddr = 0x80347130u;
constexpr std::uint32_t kThreadAttrSourceAddr = 0x80385AA8u;

constexpr std::uint32_t kContextCrOffset = 0x80u;
constexpr std::uint32_t kContextLrOffset = 0x84u;
constexpr std::uint32_t kContextXerOffset = 0x8Cu;
constexpr std::uint32_t kContextFprOffset = 0x90u;
constexpr std::uint32_t kContextFpscrOffset = 0x194u;
constexpr std::uint32_t kContextSrr0Offset = 0x198u;
constexpr std::uint32_t kContextSrr1Offset = 0x19Cu;
constexpr std::uint32_t kContextModeOffset = 0x1A0u;
constexpr std::uint32_t kContextStateOffset = 0x1A2u;
constexpr std::uint32_t kContextGqrOffset = 0x1A4u;
constexpr std::uint32_t kContextPsfOffset = 0x1C8u;

constexpr std::uint32_t kInitialSrr1 = 0x00009032u;
constexpr std::uint32_t kThreadExitTrampoline = 0x801AA0F0u;
constexpr std::uint32_t kStackGuard = 0xDEADBABEu;

bool IsThpVideoDecoderEntry(std::uint32_t entry) noexcept {
    return entry == 0x805529A8u || entry == 0x80552A74u;
}

bool ContainsThreadAndStack(std::uint32_t threadPtr,
                            std::uint32_t alignedStack,
                            std::uint32_t stackTop,
                            std::uint32_t stackSize) noexcept {
    if (!Memory::IsInitialized() || threadPtr == 0u || alignedStack < 8u || stackTop < stackSize) {
        return false;
    }

    const std::uint32_t stackBottom = stackTop - stackSize;
    return Memory::Contains(threadPtr, kThreadSize) &&
           Memory::Contains(alignedStack - 8u, 8u) &&
           Memory::Contains(stackBottom, 4u) &&
           Memory::Contains(kThreadListHeadAddr, 4u) &&
           Memory::Contains(kThreadListTailAddr, 4u);
}

void InitializeGuestContext(CpuContext* cpu,
                            std::uint32_t threadPtr,
                            std::uint32_t entryFunc,
                            std::uint32_t stackPointer) {
    Memory::Write32(threadPtr + kContextSrr0Offset, entryFunc);
    Memory::Write32(threadPtr + 0x04u, stackPointer);
    Memory::Write32(threadPtr + kContextSrr1Offset, kInitialSrr1);
    Memory::Write32(threadPtr + kContextCrOffset, 0u);
    Memory::Write32(threadPtr + kContextXerOffset, 0u);

    Memory::Write32(threadPtr + 0x08u, cpu->gpr[2]);
    Memory::Write32(threadPtr + 0x34u, cpu->gpr[13]);

    for (std::uint32_t reg = 3u; reg <= 12u; ++reg) {
        Memory::Write32(threadPtr + reg * 4u, 0u);
    }
    for (std::uint32_t reg = 14u; reg <= 31u; ++reg) {
        Memory::Write32(threadPtr + reg * 4u, 0u);
    }
    for (std::uint32_t gqr = 0u; gqr < 8u; ++gqr) {
        Memory::Write32(threadPtr + kContextGqrOffset + gqr * 4u, 0u);
    }

    // OSInitContext tail-calls OSClearContext. Preserve its guest-visible state
    // without introducing a second translated/native boundary inside this HLE.
    Memory::Write16(threadPtr + kContextModeOffset, 0u);
    Memory::Write16(threadPtr + kContextStateOffset, 0u);
    if (Memory::Contains(kCurrentFpuContextAddr, 4u) &&
        Memory::Read32(kCurrentFpuContextAddr) == threadPtr) {
        Memory::Write32(kCurrentFpuContextAddr, 0u);
    }
}

} // namespace

extern "C" void mkw_switch_hle_os_create_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();

    const std::uint32_t threadPtr = cpu->gpr[3];
    const std::uint32_t entryFunc = cpu->gpr[4];
    const std::uint32_t entryArg = cpu->gpr[5];
    const std::uint32_t stackTop = cpu->gpr[6];
    const std::uint32_t stackSize = cpu->gpr[7];
    const std::int32_t priority = static_cast<std::int32_t>(cpu->gpr[8]);
    const std::uint16_t attributes = static_cast<std::uint16_t>(cpu->gpr[9]);

    if (priority < 0 || priority > 31) {
        cpu->gpr[3] = 0u;
        return;
    }

    const std::uint32_t alignedStack = stackTop & 0xFFFFFFF8u;
    if (!ContainsThreadAndStack(threadPtr, alignedStack, stackTop, stackSize)) {
        cpu->gpr[3] = 0u;
        return;
    }

    try {
        Memory::Write16(threadPtr + kThreadStateOffset, 1u);
        Memory::Write16(threadPtr + kThreadAttrOffset, attributes & 1u);
        Memory::Write32(threadPtr + kThreadBasePriorityOffset,
                        static_cast<std::uint32_t>(priority));
        Memory::Write32(threadPtr + kThreadPriorityOffset,
                        static_cast<std::uint32_t>(priority));
        Memory::Write32(threadPtr + kThreadSuspendOffset, 1u);
        Memory::Write32(threadPtr + kThreadExitValueOffset, 0xFFFFFFFFu);

        Memory::Write32(threadPtr + kThreadMutexOffset, 0u);
        Memory::Write32(threadPtr + kThreadJoinQueueOffset + 4u, 0u);
        Memory::Write32(threadPtr + kThreadJoinQueueOffset, 0u);
        Memory::Write32(threadPtr + kThreadMutexTailOffset, 0u);
        Memory::Write32(threadPtr + kThreadMutexQueueOffset, 0u);

        Memory::Write32(alignedStack - 8u, 0u);
        Memory::Write32(alignedStack - 4u, 0u);

        InitializeGuestContext(cpu, threadPtr, entryFunc, alignedStack - 8u);

        if (IsThpVideoDecoderEntry(entryFunc)) {
            Memory::Write32(threadPtr + 0x1ACu, 0x00040004u);
            Memory::Write32(threadPtr + 0x1B0u, 0x00050005u);
            Memory::Write32(threadPtr + 0x1B4u, 0x00060006u);
            Memory::Write32(threadPtr + 0x1B8u, 0x00070007u);
        }

        Memory::Write32(threadPtr + kContextLrOffset, kThreadExitTrampoline);
        Memory::Write32(threadPtr + 0x0Cu, entryArg);

        Memory::Write32(threadPtr + kThreadStackTopOffset, stackTop);
        Memory::Write32(threadPtr + kThreadStackBottomOffset, stackTop - stackSize);
        Memory::Write32(stackTop - stackSize, kStackGuard);

        Memory::Write32(threadPtr + kThreadSpecific0Offset, 0u);
        Memory::Write32(threadPtr + kThreadSpecific1Offset, 0u);
        Memory::Write32(threadPtr + kThreadSpecific2Offset, 0u);

        if (Memory::Contains(kSchedulerInitFlagAddr, 4u) &&
            Memory::Read32(kSchedulerInitFlagAddr) != 0u) {
            Memory::Write32(threadPtr + kContextSrr1Offset,
                            Memory::Read32(threadPtr + kContextSrr1Offset) | 0x900u);
            Memory::Write16(threadPtr + kContextStateOffset,
                            static_cast<std::uint16_t>(
                                Memory::Read16(threadPtr + kContextStateOffset) | 0x1u));

            if (Memory::Contains(kThreadAttrSourceAddr, 4u)) {
                const std::uint32_t attr = (Memory::Read32(kThreadAttrSourceAddr) & 0xF8u) | 0x4u;
                Memory::Write32(threadPtr + kContextFpscrOffset, attr);
            }

            for (std::uint32_t offset = 0u; offset < 0x80u; offset += 4u) {
                Memory::Write32(threadPtr + kContextFprOffset + offset, 0u);
                Memory::Write32(threadPtr + kContextPsfOffset + offset, 0u);
            }
        }

        mkw_switch_hle_os_disable_interrupts(cpu);
        const std::uint32_t irqState = cpu->gpr[3];

        const std::uint32_t tailThread = Memory::Read32(kThreadListTailAddr);
        std::uint32_t newHead = threadPtr;
        if (tailThread != 0u) {
            Memory::Write32(tailThread + kThreadListNextOffset, threadPtr);
            newHead = Memory::Read32(kThreadListHeadAddr);
        }

        Memory::Write32(kThreadListHeadAddr, newHead);
        Memory::Write32(threadPtr + kThreadListPrevOffset, tailThread);
        Memory::Write32(threadPtr + kThreadListNextOffset, 0u);
        Memory::Write32(kThreadListTailAddr, threadPtr);

        cpu->gpr[3] = irqState;
        mkw_switch_hle_os_restore_interrupts(cpu);

        // Pinned WiiCompiled associates each created OSThread with a cooperative
        // HostContext when the guest fiber layer is active. The Switch layer
        // adopts runtime_bootstrap's already-validated scheduler and creates
        // only this thread's host stack. Failure remains host-only and does not
        // change the guest OSCreateThread success contract; SelectThread retains
        // the existing OSLoadContext fallback for a thread without a host stack.
        (void)mkw::switch_guest_fiber::create(
            threadPtr, entryFunc, entryArg, alignedStack - 8u, cpu);

        mkw_switch_note_guest_thread_event(
            "create",
            cpu,
            threadPtr,
            entryFunc,
            entryArg,
            priority);

        cpu->gpr[3] = 1u;
    } catch (...) {
        cpu->gpr[3] = 0u;
    }
}

#endif
