// Reuse the diagnostic transport/SD seams while exercising its actual reporter
// and cancellation checker. The ordinary frontier cases remain in that suite.
#define main ordinary_frontier_main
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
#include "mkw_debug_frontier_contract.cpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#undef main

#include "mkw_execution_cancel.hpp"
#include <mutex>

namespace mkw::debug {
bool reload_requested() noexcept {
    // The worker request arrives only after the real reporter has fsynced its
    // evidence and entered the hold. No new guest dispatch is allowed afterwards.
    return polls != 0;
}
} // namespace mkw::debug

int main() {
    std::filesystem::create_directories(directory);
    debug_enabled = true;
    CpuContext cpu{};
    cpu.pc = 0x8019b964;
    cpu.gpr[3] = 0x12345678;
    cpu.fpr[14].raw = 0xabcdef;
    const CpuContext before = cpu;
    const auto before_fp = MkwGetHostFpControl();
    std::mutex locked;
    bool cancelled = false;
    mkw_switch_note_translated_dispatch(0x81234560, &cpu);
    try {
        CpuContextScope cpu_scope(&cpu);
        PpcNonvolatileFprGuard fprs(&cpu);
        std::lock_guard lock(locked);
        mkw_switch_report_unsupported_translated_dispatch("TEST_FRONTIER", 0x81234560, &cpu);
        assert(false);
    } catch (const mkw::execution::ExecutionCancelled&) {
        cancelled = true;
    }
    assert(cancelled && polls == 1 && sleeps == 0 && durable_writes > 0);
    assert(TryGetCpuContext() == nullptr && MkwGetHostFpControl() == before_fp);
    assert(std::memcmp(&before, &cpu, sizeof(cpu)) == 0);
    assert(locked.try_lock());
    locked.unlock();
    assert(blocker().find("TEST_FRONTIER") != std::string::npos);
    assert(streamed.find("TEST_FRONTIER") != std::string::npos);
    std::puts("PASS: actual durable frontier hold consumes owner reload request through typed unwind and releases CPU/FPR/mutex scopes");
}
