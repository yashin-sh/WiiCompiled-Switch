#include "abi_bridge.h"
#include "memory.h"
#include "mkw_execution_cancel.hpp"
#include "switch_guest_fiber.hpp"
#include "switch_host_context_ext.hpp"

#include <ucontext.h>
#include <sanitizer/common_interface_defs.h>

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <string_view>
#include <unordered_map>

namespace {
struct Context {
    ucontext_t machine{};
    void* stack = nullptr;
    std::size_t size = 0;
    HostContext::Entry entry = nullptr;
    void* argument = nullptr;
    void* fake_stack = nullptr;
};
Context* current = nullptr;
Context* previous = nullptr;
unsigned created = 0, destroyed = 0, guest_entries = 0, destructors = 0;
bool requested = false;
bool cycle = false;
bool normal_callback = false;
constexpr std::uint32_t owner_thread = 0x80010000u;
constexpr std::uint32_t parked_thread = 0x80011000u;
constexpr std::uint32_t parent_thread = 0x80012000u;
constexpr std::uint32_t child_thread = 0x80013000u;
constexpr std::uint32_t unused_thread = 0x80014000u;
constexpr std::uint32_t parked_entry = 0x12340000u;
constexpr std::uint32_t parent_entry = 0x12340004u;
constexpr std::uint32_t child_entry = 0x12340008u;
std::array<std::mutex, 3> locks;
std::array<CpuContext*, 3> guest_cpus{};
CpuContext* transient_callback = nullptr;
std::unordered_map<std::uint32_t, std::uint32_t> words;

void FinishSwitch(Context* self) {
    const void* old_bottom = nullptr;
    std::size_t old_size = 0;
    __sanitizer_finish_switch_fiber(self->fake_stack, &old_bottom, &old_size);
    previous->stack = const_cast<void*>(old_bottom);
    previous->size = old_size;
}
void Trampoline(std::uintptr_t pointer) {
    auto* self = reinterpret_cast<Context*>(pointer);
    FinishSwitch(self);
    self->entry(self->argument);
    std::abort();
}
struct DestructionWitness {
    ~DestructionWitness() {
        ++destructors;
    }
};
void MakeGuest(std::uint32_t thread, std::uint32_t entry, CpuContext* cpu) {
    words[thread + 4] = 0x80020000u;
    words[thread + 8] = 0x80003000u;
    words[thread + 0x34] = 0x80004000u;
    words[thread + 0x84] = 0;
    assert(mkw::switch_guest_fiber::create(thread, entry, 0, 0x80020000u, cpu));
}
void NestedCallback() {
    // This object dies before drain_cancelled: keeping its pointer in a guest
    // record would make the subsequent sibling unwind a use-after-return.
    CpuContext callback_cpu{};
    transient_callback = &callback_cpu;
    for (unsigned i = 14; i <= 31; ++i)
        callback_cpu.fpr[i].raw = 0xaaa00000u + i;
    bool caught = false;
    try {
        CpuContextScope scope(&callback_cpu);
        mkw::switch_guest_fiber::switch_to(parent_thread, &callback_cpu);
        assert(false);
    } catch (const mkw::execution::ExecutionCancelled&) {
        caught = true;
        for (unsigned i = 14; i <= 31; ++i)
            assert(callback_cpu.fpr[i].raw == 0xaaa00000u + i);
    }
    assert(caught);
    transient_callback = nullptr;
}
} // namespace

namespace HostContext {
bool InitializeScheduler(Handle* scheduler) {
    assert(!current);
    auto* context = new Context;
    assert(getcontext(&context->machine) == 0);
    current = context;
    *scheduler = context;
    return true;
}
void ShutdownScheduler(Handle scheduler) {
    assert(current == scheduler);
    delete current;
    current = nullptr;
}
Handle Current() noexcept {
    return current;
}
Handle Create(std::size_t size, Entry entry, void* argument) {
    auto* context = new Context;
    context->stack = std::malloc(size);
    assert(context->stack);
    context->size = size;
    context->entry = entry;
    context->argument = argument;
    assert(getcontext(&context->machine) == 0);
    context->machine.uc_stack.ss_sp = context->stack;
    context->machine.uc_stack.ss_size = size;
    makecontext(&context->machine, reinterpret_cast<void (*)()>(&Trampoline), 1,
                reinterpret_cast<std::uintptr_t>(context));
    ++created;
    return context;
}
void Destroy(Handle handle) {
    auto* context = static_cast<Context*>(handle);
    assert(context != current);
    std::free(context->stack);
    delete context;
    ++destroyed;
}
bool IsCurrent(Handle context) {
    return current == context;
}
void Switch(Handle handle) {
    // A terminal exception must leave its catch before another guest stack
    // resumes: C++ EH bookkeeping is shared by all fibres on this thread.
    assert(std::uncaught_exceptions() == 0);
    assert(!std::current_exception());
    auto* from = current;
    auto* to = static_cast<Context*>(handle);
    assert(from != to);
    current = to;
    previous = from;
    __sanitizer_start_switch_fiber(&from->fake_stack, to->stack, to->size);
    assert(swapcontext(&from->machine, &to->machine) == 0);
    FinishSwitch(from);
}
} // namespace HostContext

namespace mkw::execution {
void check_execution_cancel() {
    if (requested)
        throw ExecutionCancelled{};
}
} // namespace mkw::execution

std::uint32_t Memory::Read32(std::uint32_t address) {
    return words.at(address);
}
extern "C" void mkw_switch_note_guest_thread_event(const char*, CpuContext*, std::uint32_t,
                                                   std::uint32_t, std::uint32_t, std::int32_t) noexcept {}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t, CpuContext*) {
    mkw::execution::check_execution_cancel();
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) {}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char*, std::uint32_t, CpuContext*) {
    assert(false); // Only the typed cancellation route is exercised here.
    std::abort();
}
bool mkw_switch_try_dispatch_indirect(std::uint32_t target, CpuContext* cpu) {
    ++guest_entries;
    PpcNonvolatileFprGuard fprs(cpu);
    const unsigned index = target == parked_entry ? 0 : target == parent_entry ? 1
                                                                               : 2;
    assert(cpu != transient_callback);
    assert(TryGetCpuContext() == cpu);
    guest_cpus[index] = cpu;
    std::lock_guard lock(locks[index]);
    DestructionWitness witness;
    cpu->fpr[14].raw = 0xdeadbeef;
    MkwApplyHostNiMode(4u);
    const auto source_fp = MkwGetHostFpControl();
    if (target == parked_entry) {
        words[0x800000d4u] = words[0x800000e4u] = owner_thread;
        if (normal_callback) {
            cpu->gpr[9] = 0x91919191u;
            CpuContext callback_cpu = *cpu;
            callback_cpu.gpr[9] = 0xccccccccu;
            {
                CpuContextScope callback_scope(&callback_cpu);
                mkw::switch_guest_fiber::switch_to(owner_thread, &callback_cpu);
                assert(TryGetCpuContext() == &callback_cpu);
                assert(callback_cpu.gpr[9] == 0xccccccccu);
                assert(cpu->gpr[9] == 0x91919191u);
            }
            assert(TryGetCpuContext() == cpu && cpu->gpr[9] == 0x91919191u);
            requested = true;
            mkw::execution::check_execution_cancel();
        }
        mkw::switch_guest_fiber::switch_to(owner_thread, cpu);
    } else if (target == parent_entry) {
        mkw::switch_guest_fiber::switch_to(child_thread, cpu);
        assert(cycle);
        assert(MkwGetHostFpControl() == source_fp);
        requested = true;
        mkw::execution::check_execution_cancel();
    } else {
        assert(target == child_entry);
        if (cycle) {
            words[0x800000d4u] = words[0x800000e4u] = parent_thread;
            mkw::switch_guest_fiber::switch_to(parent_thread, cpu);
            assert(false);
        }
        requested = true;
        mkw::execution::check_execution_cancel();
    }
    assert(false); // No parked/nested guest may run beyond cancellation.
    return true;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    cycle = std::string_view(argv[1]) == "cycle";
    normal_callback = std::string_view(argv[1]) == "callback";
    assert(cycle || normal_callback || std::string_view(argv[1]) == "nested");
    HostContext::Handle scheduler = nullptr;
    assert(HostContext::InitializeScheduler(&scheduler));
    auto& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.fpscr = 0;
    cpu.fpr[14].raw = 0x11223344;
    const auto owner_fp = MkwGetHostFpControl();
    assert(mkw::switch_guest_fiber::initialize_from_current_host());
    assert(mkw::switch_guest_fiber::register_current(owner_thread, &cpu));
    MakeGuest(parked_thread, parked_entry, &cpu);
    MakeGuest(parent_thread, parent_entry, &cpu);
    MakeGuest(child_thread, child_entry, &cpu);
    MakeGuest(unused_thread, child_entry, &cpu);
    assert(mkw::switch_guest_fiber::switch_to(parked_thread, &cpu));
    assert(guest_entries == 1 && destructors == 0);
    assert(!mkw::switch_guest_fiber::create(parked_thread, parked_entry, 0, 0x80020000u, &cpu));
    assert(!mkw::switch_guest_fiber::shutdown_quiescent());
    assert(destroyed == 0);
    if (normal_callback) {
        cpu.gpr[9] = 0xbbbbbbbb;
        bool cancelled = false;
        try {
            CpuContextScope scope(&cpu);
            mkw::switch_guest_fiber::switch_to(parked_thread, &cpu);
            assert(false);
        } catch (const mkw::execution::ExecutionCancelled&) {
            cancelled = true;
        }
        assert(cancelled && cpu.gpr[9] == 0xbbbbbbbb);
        assert(guest_entries == 1 && destructors == 1);
        assert(TryGetCpuContext() == nullptr);
        assert(mkw::switch_guest_fiber::drain_cancelled());
        assert(mkw::switch_guest_fiber::shutdown_quiescent());
        assert(created == 4 && destroyed == 4);
        HostContext::ShutdownScheduler(scheduler);
        std::puts("PASS: ordinary callback yield/resume preserves separate root and automatic callback CPU contexts before cancellation");
        return 0;
    }
    NestedCallback();
    assert(guest_cpus[0] != guest_cpus[1] && guest_cpus[1] != guest_cpus[2] && guest_cpus[0] != guest_cpus[2]);
    assert(guest_entries == 3 && destructors == 2);
    assert(TryGetCpuContext() == nullptr);
    assert(mkw::switch_guest_fiber::drain_cancelled());
    assert(cpu.fpr[14].raw == 0x11223344);
    assert(guest_entries == 3 && destructors == 3); // Unstarted fibre skipped.
    assert(TryGetCpuContext() == nullptr);
    assert(MkwGetHostFpControl() == owner_fp);
    for (auto& mutex : locks) {
        assert(mutex.try_lock());
        mutex.unlock();
    }
    assert(mkw::switch_guest_fiber::shutdown_quiescent());
    assert(created == 4 && destroyed == 4);
    HostContext::ShutdownScheduler(scheduler);
    std::puts("PASS: real guest fibre cancellation unwinds nested and parked stacks, clears EH before stack switches, skips unstarted fibres and frees only quiescent contexts");
}
