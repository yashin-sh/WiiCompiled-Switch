#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstdlib>

namespace {

constexpr std::uint32_t kOsDetachThreadAddress = 0x801AA4ECu;
constexpr std::uint32_t kObservedThread = 0x901187C0u;

constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadAttrOffset = 0x2CAu;
constexpr std::uint32_t kThreadJoinQueueOffset = 0x2E8u;

constexpr std::uint16_t kObservedState = 4u;
constexpr std::uint16_t kObservedAttr = 0x0001u;

[[noreturn]] void AbortUnproven(CpuContext* cpu, const char* kind) noexcept {
    if (cpu) {
        cpu->gpr[3] = kObservedThread;
    }
    mkw_switch_report_unsupported_translated_dispatch(
        kind,
        kOsDetachThreadAddress,
        cpu);
    std::abort();
}

bool ContainsObservedThread() noexcept {
    return Memory::IsInitialized() &&
           Memory::Contains(kObservedThread + kThreadStateOffset, sizeof(std::uint16_t)) &&
           Memory::Contains(kObservedThread + kThreadAttrOffset, sizeof(std::uint16_t)) &&
           Memory::Contains(kObservedThread + kThreadJoinQueueOffset, 8u);
}

} // namespace

extern "C" void mkw_switch_hle_os_detach_thread(CpuContext* ctx) noexcept {
    CpuContext* cpu = ctx ? ctx : &GetPersistentCpuContext();
    const std::uint32_t threadPtr = cpu->gpr[3];

    if (threadPtr != kObservedThread || !ContainsObservedThread()) {
        AbortUnproven(cpu, "OSDETACHTHREAD_UNPROVEN_THREAD");
    }

    try {
        const std::uint16_t state = Memory::Read16(threadPtr + kThreadStateOffset);
        const std::uint16_t attr = Memory::Read16(threadPtr + kThreadAttrOffset);
        const std::uint32_t joinHead =
            Memory::Read32(threadPtr + kThreadJoinQueueOffset);
        const std::uint32_t joinTail =
            Memory::Read32(threadPtr + kThreadJoinQueueOffset + 4u);

        // Hardware proves only this first TaskThread detach path:
        // WAITING, already detached, and no joiners. Do not generalize to the
        // pinned MORIBUND cleanup branch or to a different thread instance.
        if (state != kObservedState ||
            attr != kObservedAttr ||
            joinHead != 0u ||
            joinTail != 0u) {
            AbortUnproven(cpu, "OSDETACHTHREAD_UNPROVEN_STATE");
        }

        mkw_switch_hle_os_disable_interrupts(cpu);
        const std::uint32_t irqState = cpu->gpr[3];

        // Pinned OSDetachThread sets the detached bit unconditionally.
        Memory::Write16(
            threadPtr + kThreadAttrOffset,
            static_cast<std::uint16_t>(attr | 1u));

        // state == WAITING, so the pinned MORIBUND delist/fiber-termination
        // branch is not entered. WakeThreadJoiners still runs; with the exact
        // observed empty join queue this is a no-op apart from its nested
        // interrupt bookkeeping, which the existing OSWakeupThread HLE owns.
        cpu->gpr[3] = threadPtr + kThreadJoinQueueOffset;
        mkw_switch_hle_os_wakeup_thread(cpu);

        cpu->gpr[3] = irqState;
        mkw_switch_hle_os_restore_interrupts(cpu);
    } catch (...) {
        AbortUnproven(cpu, "OSDETACHTHREAD_GUEST_MEMORY");
    }
}

#endif
