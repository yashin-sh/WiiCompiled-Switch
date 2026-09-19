#include "fast_track_thread_diagnostics.hpp"

#include "abi_bridge.h"

#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <cstdint>
#include <cstdio>

#if defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION

namespace {

constexpr const char* kThreadEventPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-thread-events.txt";
constexpr std::uint32_t kMaxThreadEvents = 64u;

constexpr std::uint32_t kOsCurrentContextAddr = 0x800000D4u;
constexpr std::uint32_t kOsRunningContextAddr = 0x800000E4u;

constexpr std::uint32_t kContextR1Offset = 0x04u;
constexpr std::uint32_t kContextR3Offset = 0x0Cu;
constexpr std::uint32_t kContextSrr0Offset = 0x198u;
constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadSuspendOffset = 0x2CCu;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadBasePriorityOffset = 0x2D4u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;

std::uint32_t g_threadEventCount = 0u;

std::uint32_t Read32OrZero(std::uint32_t address) noexcept {
    try {
        if (Memory::IsInitialized() && Memory::Contains(address, 4u)) {
            return Memory::Read32(address);
        }
    } catch (...) {
    }
    return 0u;
}

std::uint16_t Read16OrZero(std::uint32_t address) noexcept {
    try {
        if (Memory::IsInitialized() && Memory::Contains(address, 2u)) {
            return Memory::Read16(address);
        }
    } catch (...) {
    }
    return 0u;
}

void ReadEggVtable(
    std::uint32_t objectPtr,
    std::uint32_t& vtable,
    std::uint32_t& dtor,
    std::uint32_t& run,
    std::uint32_t& onExit,
    std::uint32_t& onEnter) noexcept {
    vtable = Read32OrZero(objectPtr);
    if (vtable == 0u) {
        dtor = 0u;
        run = 0u;
        onExit = 0u;
        onEnter = 0u;
        return;
    }

    dtor = Read32OrZero(vtable + 0x08u);
    run = Read32OrZero(vtable + 0x0Cu);
    onExit = Read32OrZero(vtable + 0x10u);
    onEnter = Read32OrZero(vtable + 0x14u);
}

} // namespace

#endif

extern "C" void mkw_switch_note_guest_thread_event(
    const char* kind,
    CpuContext* cpu,
    std::uint32_t threadPtr,
    std::uint32_t entryPoint,
    std::uint32_t entryArg,
    std::int32_t requestedPriority) noexcept {
#if defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION
    if (g_threadEventCount >= kMaxThreadEvents) {
        return;
    }

    ++g_threadEventCount;

    const std::uint32_t storedEntry =
        threadPtr != 0u ? Read32OrZero(threadPtr + kContextSrr0Offset) : 0u;
    const std::uint32_t storedArg =
        threadPtr != 0u ? Read32OrZero(threadPtr + kContextR3Offset) : 0u;
    const std::uint32_t storedR1 =
        threadPtr != 0u ? Read32OrZero(threadPtr + kContextR1Offset) : 0u;
    const std::uint32_t effectiveEntry =
        entryPoint != 0u ? entryPoint : storedEntry;
    const std::uint32_t effectiveArg =
        entryArg != 0u ? entryArg : storedArg;

    std::uint32_t vtable = 0u;
    std::uint32_t dtor = 0u;
    std::uint32_t run = 0u;
    std::uint32_t onExit = 0u;
    std::uint32_t onEnter = 0u;
    ReadEggVtable(effectiveArg, vtable, dtor, run, onExit, onEnter);

    const std::uint32_t state =
        threadPtr != 0u ? Read16OrZero(threadPtr + kThreadStateOffset) : 0u;
    const std::uint32_t suspend =
        threadPtr != 0u ? Read32OrZero(threadPtr + kThreadSuspendOffset) : 0u;
    const std::uint32_t priority =
        threadPtr != 0u ? Read32OrZero(threadPtr + kThreadPriorityOffset) : 0u;
    const std::uint32_t basePriority =
        threadPtr != 0u ? Read32OrZero(threadPtr + kThreadBasePriorityOffset) : 0u;
    const std::uint32_t queue =
        threadPtr != 0u ? Read32OrZero(threadPtr + kThreadQueueOffset) : 0u;

    FILE* out = std::fopen(
        kThreadEventPath,
        g_threadEventCount == 1u ? "w" : "a");
    if (!out) {
        return;
    }

    std::fprintf(
        out,
        "event=%u kind=%s thread=0x%08x entry=0x%08x arg=0x%08x "
        "requested_prio=%d state=%u suspend=%u prio=%u base_prio=%u "
        "queue=0x%08x stored_r1=0x%08x fiber=0x%08x "
        "os_current=0x%08x os_running=0x%08x "
        "vtable=0x%08x vt_dtor=0x%08x vt_run=0x%08x "
        "vt_on_exit=0x%08x vt_on_enter=0x%08x "
        "cpu_pc=0x%08x cpu_r1=0x%08x cpu_r3=0x%08x\n",
        g_threadEventCount,
        kind ? kind : "<null>",
        threadPtr,
        effectiveEntry,
        effectiveArg,
        static_cast<int>(requestedPriority),
        state,
        suspend,
        priority,
        basePriority,
        queue,
        storedR1,
        mkw::switch_guest_fiber::current_thread(),
        Read32OrZero(kOsCurrentContextAddr),
        Read32OrZero(kOsRunningContextAddr),
        vtable,
        dtor,
        run,
        onExit,
        onEnter,
        cpu ? cpu->pc : 0u,
        cpu ? cpu->gpr[1] : 0u,
        cpu ? cpu->gpr[3] : 0u);
    std::fclose(out);
#else
    (void)kind;
    (void)cpu;
    (void)threadPtr;
    (void)entryPoint;
    (void)entryArg;
    (void)requestedPriority;
#endif
}
