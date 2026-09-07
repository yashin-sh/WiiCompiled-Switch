#include "context_probe.hpp"

#include <switch.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

extern "C" void mkw_switch_co_switch(void** target_sp, void** source_sp);
extern "C" void* mkw_switch_co_init(void* stack_top, void (*entry)(void*), void* argument);
extern "C" int mkw_switch_co_register_roundtrip(void** scheduler_sp, void** worker_sp);

namespace mkw::context_probe {
namespace {

constexpr std::size_t kWorkerStackSize = 256 * 1024;
constexpr std::uint32_t kStressSwitches = 100'000;

void* g_scheduler_sp = nullptr;
void* g_worker_sp = nullptr;
volatile std::uint32_t g_phase = 0;
volatile std::uint32_t g_stress_count = 0;
volatile bool g_register_preservation_ok = false;

void worker_entry(void*) {
    g_phase = 1;
    mkw_switch_co_switch(&g_scheduler_sp, &g_worker_sp);

    g_phase = 2;
    mkw_switch_co_switch(&g_scheduler_sp, &g_worker_sp);

    // The assembly helper itself performs one worker -> scheduler -> worker
    // round-trip while sentinel values live in AAPCS64 callee-saved registers.
    g_phase = 3;
    g_register_preservation_ok =
        mkw_switch_co_register_roundtrip(&g_scheduler_sp, &g_worker_sp) == 1;

    g_phase = 4;
    mkw_switch_co_switch(&g_scheduler_sp, &g_worker_sp);

    for (std::uint32_t i = 1; i <= kStressSwitches; ++i) {
        g_stress_count = i;
        mkw_switch_co_switch(&g_scheduler_sp, &g_worker_sp);
    }

    g_phase = 5;
    mkw_switch_co_switch(&g_scheduler_sp, &g_worker_sp);

    // A cooperative worker must never return through the bootstrap trampoline.
    for (;;) {
        mkw_switch_co_switch(&g_scheduler_sp, &g_worker_sp);
    }
}

void emit(FILE* out, const ProbeResult& r) {
    std::fprintf(out, "\nWiiCompiled-Switch AArch64 context probe\n");
    std::fprintf(out, "=======================================\n");
    std::fprintf(out, "worker stack          : %s\n", r.stack_allocated ? "OK" : "FAILED");
    std::fprintf(out, "context init          : %s\n", r.context_initialized ? "OK" : "FAILED");
    std::fprintf(out, "first handoff         : %s\n", r.first_handoff_ok ? "OK" : "FAILED");
    std::fprintf(out, "continuation          : %s\n", r.continuation_ok ? "OK" : "FAILED");
    std::fprintf(out, "callee-saved registers: %s\n", r.register_preservation_ok ? "OK" : "FAILED");
    std::fprintf(out, "stress switches       : %u / %u\n",
                 r.completed_switches, kStressSwitches);
    std::fprintf(out, "stress result         : %s\n", r.stress_ok ? "OK" : "FAILED");
    std::fprintf(out, "stress elapsed ticks  : %llu\n",
                 static_cast<unsigned long long>(r.elapsed_ticks));
}

} // namespace

ProbeResult run() {
    ProbeResult r{};
    g_scheduler_sp = nullptr;
    g_worker_sp = nullptr;
    g_phase = 0;
    g_stress_count = 0;
    g_register_preservation_ok = false;

    void* stack = memalign(0x1000, kWorkerStackSize);
    if (!stack) {
        return r;
    }
    r.stack_allocated = true;
    std::memset(stack, 0, kWorkerStackSize);

    void* stack_top = static_cast<unsigned char*>(stack) + kWorkerStackSize;
    g_worker_sp = mkw_switch_co_init(stack_top, worker_entry, nullptr);
    if (!g_worker_sp) {
        std::free(stack);
        return r;
    }
    r.context_initialized = true;

    mkw_switch_co_switch(&g_worker_sp, &g_scheduler_sp);
    r.first_handoff_ok = g_phase == 1;
    if (!r.first_handoff_ok) {
        std::free(stack);
        return r;
    }

    mkw_switch_co_switch(&g_worker_sp, &g_scheduler_sp);
    r.continuation_ok = g_phase == 2;
    if (!r.continuation_ok) {
        std::free(stack);
        return r;
    }

    // First half of the register round-trip: the assembly helper switches
    // back here with sentinel register values saved in the worker context.
    mkw_switch_co_switch(&g_worker_sp, &g_scheduler_sp);
    if (g_phase != 3) {
        std::free(stack);
        return r;
    }

    // Resume the assembly helper so it can validate the restored values, then
    // let worker_entry report the result back to us.
    mkw_switch_co_switch(&g_worker_sp, &g_scheduler_sp);
    r.register_preservation_ok = g_phase == 4 && g_register_preservation_ok;
    if (!r.register_preservation_ok) {
        std::free(stack);
        return r;
    }

    const std::uint64_t start_ticks = armGetSystemTick();
    bool sequence_ok = true;
    for (std::uint32_t i = 1; i <= kStressSwitches; ++i) {
        mkw_switch_co_switch(&g_worker_sp, &g_scheduler_sp);
        if (g_stress_count != i) {
            sequence_ok = false;
            break;
        }
        r.completed_switches = i;
    }
    r.elapsed_ticks = armGetSystemTick() - start_ticks;

    if (sequence_ok && r.completed_switches == kStressSwitches) {
        // Resume once after the last stress yield so the worker can reach its
        // explicit terminal phase and yield again without returning.
        mkw_switch_co_switch(&g_worker_sp, &g_scheduler_sp);
        r.stress_ok = g_phase == 5;
    }

    // The worker is suspended and will never be resumed after this point, so
    // its backing stack can be reclaimed safely.
    std::free(stack);
    return r;
}

void print(const ProbeResult& result) {
    emit(stdout, result);
    std::printf("\n");
    consoleUpdate(nullptr);
}

bool append_report(const ProbeResult& result) {
    FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/vm-probe.txt", "a");
    if (!out) {
        return false;
    }
    emit(out, result);
    std::fclose(out);
    return true;
}

} // namespace mkw::context_probe
