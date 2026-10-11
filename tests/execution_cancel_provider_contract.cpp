#include "abi_bridge.h"
#include "mkw_debug_server.hpp"
#include "mkw_execution_cancel.hpp"
#include "switch_guest_fiber.hpp"
#include "translated_execution_handoff.hpp"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <string>
static bool owner_ready = false, drain_ok = true;
static unsigned mode = 0, drains = 0, guards = 0;
struct Guard {
    ~Guard() {
        ++guards;
    }
};
namespace mkw::debug {
void set_reload_owner_available(bool value) noexcept {
    owner_ready = value;
}
} // namespace mkw::debug
namespace mkw::switch_guest_fiber {
bool drain_cancelled() noexcept {
    assert(TryGetCpuContext() == nullptr);
    ++drains;
    return drain_ok;
}
} // namespace mkw::switch_guest_fiber
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}
extern "C" void func_800060A4(CpuContext* cpu) {
    assert(TryGetCpuContext() == cpu && owner_ready);
    Guard guard;
    PpcNonvolatileFprGuard fpr(cpu);
    cpu->fpr[14].raw = 0x12345678;
    if (mode == 0)
        throw mkw::execution::ExecutionCancelled{};
    if (mode == 1)
        throw std::runtime_error("synthetic fault");
    if (mode == 2)
        throw 12;
}
static void ProgressContains(const char* phase) {
    std::ifstream in("sdmc:/switch/WiiCompiled-Switch/fast-track-progress.txt");
    std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    assert(text.find(phase) != std::string::npos);
}
int main() {
    std::filesystem::create_directories("sdmc:/switch/WiiCompiled-Switch");
    const auto fp = MkwGetHostFpControl();
    auto status = mkw::translated_execution_handoff::inspect();
    assert(status.abi_compatible && status.reported_abi == 2 && status.runner_available);
    MkwSwitchTranslatedExecutionProbeResult result{};
    assert(!mkw::translated_execution_handoff::run_first_translated_function(result));
    assert(result.cancelled && result.guest_quiescent && owner_ready && drains == 1 && guards == 1);
    ProgressContains("CANCELLED_AND_QUIESCENT");
    assert(TryGetCpuContext() == nullptr && MkwGetHostFpControl() == fp);
    mode = 1;
    assert(!mkw::translated_execution_handoff::run_first_translated_function(result));
    assert(!result.cancelled && !result.guest_quiescent && !owner_ready && drains == 1 && guards == 2);
    ProgressContains("BLOCKED_STD_EXCEPTION");
    mode = 2;
    assert(!mkw::translated_execution_handoff::run_first_translated_function(result));
    assert(!result.cancelled && !owner_ready && guards == 3);
    ProgressContains("BLOCKED_UNKNOWN_EXCEPTION");
    mode = 3;
    assert(mkw::translated_execution_handoff::run_first_translated_function(result));
    assert(!result.cancelled && !result.guest_quiescent && owner_ready && guards == 4);
    ProgressContains("RETURNED_FROM_TRANSLATED_START");
    mode = 0;
    drain_ok = false;
    assert(!mkw::translated_execution_handoff::run_first_translated_function(result));
    assert(result.cancelled && !result.guest_quiescent && !owner_ready && drains == 2 && guards == 5);
    ProgressContains("CANCELLATION_DRAIN_FAILED");
    assert(TryGetCpuContext() == nullptr && MkwGetHostFpControl() == fp);
    std::puts("PASS: actual ABI2 provider/wrapper handles typed cancellation, natural return, std/unknown faults and failed drain without false readiness");
}
