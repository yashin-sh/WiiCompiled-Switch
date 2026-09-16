#include "switch_guest_fiber.hpp"

#include "devkita64_gcc_compat.hpp"
#include "abi_bridge.h"
#include "memory.h"
#include "switch_host_context_ext.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace mkw::switch_guest_fiber {
namespace {

constexpr std::size_t kMaxGuestFibers = 64u;
constexpr std::size_t kHostStackSize = 1024u * 1024u;
constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;

constexpr std::uint32_t kContextR1Offset = 0x04u;
constexpr std::uint32_t kContextR2Offset = 0x08u;
constexpr std::uint32_t kContextR13Offset = 0x34u;
constexpr std::uint32_t kContextLrOffset = 0x84u;

enum class FiberState : std::uint8_t {
    Empty,
    Ready,
    Running,
    Waiting,
};

struct GuestFiberRecord {
    std::uint32_t guest_thread = 0u;
    HostContext::Handle host = nullptr;
    std::uint32_t entry_point = 0u;
    std::uint32_t entry_arg = 0u;
    CpuContext saved_cpu{};
    FiberState state = FiberState::Empty;
    bool scheduler_host = false;
};

std::array<GuestFiberRecord, kMaxGuestFibers> g_records{};
HostContext::Handle g_scheduler_host = nullptr;
std::uint32_t g_current_guest_thread = 0u;
CpuContext* g_active_cpu = nullptr;

GuestFiberRecord* Find(std::uint32_t guest_thread) noexcept {
    if (guest_thread == 0u) {
        return nullptr;
    }
    for (auto& record : g_records) {
        if (record.state != FiberState::Empty && record.guest_thread == guest_thread) {
            return &record;
        }
    }
    return nullptr;
}

GuestFiberRecord* Allocate(std::uint32_t guest_thread) noexcept {
    if (auto* existing = Find(guest_thread)) {
        return existing;
    }
    for (auto& record : g_records) {
        if (record.state == FiberState::Empty) {
            record = {};
            record.guest_thread = guest_thread;
            return &record;
        }
    }
    return nullptr;
}

[[noreturn]] void AbortGuestFiberBoundary(const char* kind, std::uint32_t target,
                                           CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(kind, target, cpu);
    std::abort();
}

void GuestFiberEntry(void* argument) {
    const std::uint32_t guest_thread =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(argument));
    GuestFiberRecord* record = Find(guest_thread);
    CpuContext* cpu = g_active_cpu ? g_active_cpu : &GetPersistentCpuContext();
    if (!record || !cpu) {
        AbortGuestFiberBoundary("GUEST_FIBER_ENTRY_STATE", guest_thread, cpu);
    }

    try {
        // Match the pinned GuestFiberManager's first-entry handoff. The guest
        // OSContext is authoritative for the ABI-critical registers initialized
        // by OSCreateThread; the translated entry itself starts at its real
        // function boundary, not at an invented interior trampoline.
        cpu->gpr[1] = Memory::Read32(guest_thread + kContextR1Offset);
        cpu->gpr[2] = Memory::Read32(guest_thread + kContextR2Offset);
        cpu->gpr[13] = Memory::Read32(guest_thread + kContextR13Offset);
        cpu->lr = Memory::Read32(guest_thread + kContextLrOffset);
        cpu->gpr[3] = record->entry_arg;
        cpu->pc = record->entry_point;
        cpu->srr0 = record->entry_point;

        CpuContextScope scope(cpu);
        InvokeIndirectCpu(record->entry_point, cpu);
    } catch (...) {
        AbortGuestFiberBoundary("GUEST_FIBER_ENTRY_EXCEPTION", record->entry_point, cpu);
    }

    // Full guest-thread exit semantics are a separate hardware boundary. Do not
    // guess OSExitThread cleanup here; keep an unexpected natural return durable.
    AbortGuestFiberBoundary("GUEST_FIBER_ENTRY_RETURNED", record->entry_point, cpu);
}

} // namespace

bool initialize_from_current_host() noexcept {
    if (g_scheduler_host) {
        return HostContext::Current() != nullptr;
    }
    g_scheduler_host = HostContext::Current();
    return g_scheduler_host != nullptr;
}

bool available() noexcept {
    return g_scheduler_host != nullptr && HostContext::Current() != nullptr;
}

bool create(std::uint32_t guest_thread,
            std::uint32_t entry_point,
            std::uint32_t entry_arg,
            std::uint32_t guest_stack_top,
            CpuContext* seed_cpu) noexcept {
    if (!g_scheduler_host && !initialize_from_current_host()) {
        return false;
    }
    if (!available() || guest_thread == 0u || entry_point == 0u) {
        return false;
    }

    GuestFiberRecord* record = Allocate(guest_thread);
    if (!record) {
        return false;
    }
    if (record->host && !record->scheduler_host) {
        HostContext::Destroy(record->host);
    }

    record->host = HostContext::Create(
        kHostStackSize,
        GuestFiberEntry,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(guest_thread)));
    if (!record->host) {
        *record = {};
        return false;
    }

    record->guest_thread = guest_thread;
    record->entry_point = entry_point;
    record->entry_arg = entry_arg;
    record->saved_cpu = {};
    record->saved_cpu.gpr[1] = guest_stack_top;
    record->saved_cpu.gpr[3] = entry_arg;
    record->saved_cpu.pc = entry_point;
    record->saved_cpu.srr0 = entry_point;
    record->saved_cpu.hid2 =
        seed_cpu && seed_cpu->hid2 != 0u ? seed_cpu->hid2 : 0x10000000u;
    record->state = FiberState::Waiting;
    record->scheduler_host = false;
    return true;
}

bool has(std::uint32_t guest_thread) noexcept {
    const auto* record = Find(guest_thread);
    return record && record->host;
}

std::uint32_t current_thread() noexcept {
    return g_current_guest_thread;
}

bool register_current(std::uint32_t guest_thread, CpuContext* cpu) noexcept {
    if (!available() || guest_thread == 0u || HostContext::Current() != g_scheduler_host) {
        return false;
    }

    GuestFiberRecord* record = Allocate(guest_thread);
    if (!record) {
        return false;
    }
    if (record->host && record->host != g_scheduler_host) {
        return false;
    }

    record->guest_thread = guest_thread;
    record->host = g_scheduler_host;
    record->saved_cpu = cpu ? *cpu : CpuContext{};
    record->state = FiberState::Running;
    record->scheduler_host = true;
    g_current_guest_thread = guest_thread;
    g_active_cpu = cpu;
    return true;
}

void suspend(std::uint32_t guest_thread) noexcept {
    if (auto* record = Find(guest_thread)) {
        record->state = FiberState::Waiting;
    }
}

void resume(std::uint32_t guest_thread) noexcept {
    if (auto* record = Find(guest_thread)) {
        record->state = FiberState::Ready;
    }
}

bool switch_to(std::uint32_t guest_thread, CpuContext* cpu) noexcept {
    GuestFiberRecord* target = Find(guest_thread);
    if (!available() || !target || !target->host || !cpu) {
        return false;
    }

    const std::uint32_t previous_thread = g_current_guest_thread;
    GuestFiberRecord* previous = Find(previous_thread);
    const CpuContext caller_context = *cpu;

    if (previous) {
        previous->saved_cpu = *cpu;
    }

    g_current_guest_thread = guest_thread;
    target->state = FiberState::Running;
    *cpu = target->saved_cpu;
    g_active_cpu = cpu;

    if (HostContext::IsCurrent(target->host)) {
        return true;
    }

    HostContext::Switch(target->host);

    // The source host stack resumes only when guest scheduling has selected it
    // again. Mirror the pin's bookkeeping: if guest globals confirm that fact,
    // restore this thread's most recently saved CpuContext. Otherwise restore
    // the caller snapshot and leave guest-fiber ownership unclaimed.
    try {
        const std::uint32_t running = Memory::Read32(kOSRunningContextAddr);
        const std::uint32_t current = Memory::Read32(kOSCurrentContextAddr);
        if (previous && previous_thread != 0u &&
            running == previous_thread && current == previous_thread) {
            *cpu = previous->saved_cpu;
            previous->state = FiberState::Running;
            g_current_guest_thread = previous_thread;
        } else {
            *cpu = caller_context;
            g_current_guest_thread = 0u;
        }
    } catch (...) {
        *cpu = caller_context;
        g_current_guest_thread = 0u;
    }
    g_active_cpu = cpu;
    return true;
}

} // namespace mkw::switch_guest_fiber
