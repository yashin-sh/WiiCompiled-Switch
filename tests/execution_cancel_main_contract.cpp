// Run the actual owner main with deterministic platform resources and genuine
// background threads. GPU/SDK functions are transport seams, not hardware proof.
#define main candidate_main
#include "../source/main.cpp"
#undef main

#include "abi_bridge.h"
#include "mkw_execution_cancel.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C" void userAppExit() noexcept;

namespace {
const auto owner = std::this_thread::get_id();
std::thread debug_worker, watchdog_worker, device_worker;
std::atomic<bool> requested{false}, stop_debug{false}, stop_watchdog{false};
std::atomic<bool> debug_alive{false}, watchdog_alive{false};
std::atomic<bool> device_alive{false}, stop_device{false};
bool memory_alive = false, renderer_alive = false, guest_live = false;
bool prepared = false, returned = false;
unsigned prepare_attempts = 0, destroyed_guards = 0;
unsigned debug_stop_attempts = 0, watchdog_stop_attempts = 0;
unsigned fini_calls = 0;
std::vector<std::string> events;
std::mutex guest_lock;
std::string scenario;
void Event(const char* value) {
    assert(std::this_thread::get_id() == owner);
    events.emplace_back(value);
}
struct GuestGuard {
    ~GuestGuard() {
        ++destroyed_guards;
    }
};
void Guest(CpuContext* cpu) {
    std::lock_guard lock(guest_lock);
    GuestGuard guard;
    cpu->fpr[14].raw = 0xbad;
    for (;;) {
        mkw::execution::check_execution_cancel();
        std::this_thread::yield();
    }
}
void ExitObserver() {
    assert(returned && prepared);
    assert(!debug_alive && !watchdog_alive);
    assert(!debug_worker.joinable() && !watchdog_worker.joinable());
    assert(!memory_alive && !renderer_alive && !guest_live);
    assert(destroyed_guards == 1);
    assert(events.back() == "platform-stop");
    // Model libnx's __appExit -> strong userAppExit call after exitprocs,
    // rather than pretending all Dawn references died before main returned.
    assert(device_alive && device_worker.joinable());
    userAppExit();
    userAppExit();
    assert(fini_calls == 1 && !device_alive && !device_worker.joinable());
    std::puts("PASS: actual main returned normally after guest unwind and diagnostic joins; actual libnx exit hook dispatches residual static device finalizers exactly once before loader return");
}
} // namespace

namespace mkw::execution {
void check_execution_cancel() {
    if (requested.load())
        throw ExecutionCancelled{};
}
} // namespace mkw::execution
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}
extern "C" bool mkw_switch_stop_liveness_watchdog() noexcept {
    assert(prepared && !debug_alive);
    ++watchdog_stop_attempts;
    if (scenario == "join-retry" && watchdog_stop_attempts == 1)
        return false;
    Event("watchdog-join");
    stop_watchdog = true;
    watchdog_worker.join();
    assert(!watchdog_alive);
    return true;
}
extern "C" bool mkw_switch_renderer_initialize() noexcept {
    Event("renderer-start");
    renderer_alive = true;
    device_worker = std::thread([] {
        device_alive = true;
        while (!stop_device)
            std::this_thread::yield();
        device_alive = false;
    });
    while (!device_alive)
        std::this_thread::yield();
    return true;
}
extern "C" void __libc_fini_array() {
    assert(std::this_thread::get_id() == owner && returned && prepared);
    assert(!guest_live && !memory_alive && !renderer_alive && !debug_alive && !watchdog_alive);
    ++fini_calls;
    stop_device = true;
    device_worker.join();
}
extern "C" bool mkw_switch_renderer_shutdown() noexcept {
    assert(prepared && memory_alive && !guest_live && !debug_alive && !watchdog_alive);
    Event("renderer-stop");
    renderer_alive = false;
    return true;
}
namespace mkw::debug {
void set_launch_path(const char* path) noexcept {
    assert(path && std::string(path) == "sdmc:/switch/candidate.nro");
}
bool reload_requested() noexcept {
    return requested.load();
}
void set_reload_owner_available(bool) noexcept {
    assert(false);
}
bool prepare_reload_on_owner() noexcept {
    assert(std::this_thread::get_id() == owner);
    assert(!guest_live && destroyed_guards == 1 && renderer_alive && memory_alive);
    assert(debug_alive && watchdog_alive);
    Event("prepare-loader");
    ++prepare_attempts;
    if (scenario == "retry" && prepare_attempts == 1) {
        requested = false;
        return false; // SDK failure leaves resources and listener alive.
    }
    prepared = true;
    return true;
}
bool stop() noexcept {
    assert(prepared && !guest_live && renderer_alive && memory_alive);
    ++debug_stop_attempts;
    if (scenario == "join-retry" && debug_stop_attempts == 1)
        return false;
    if (!debug_worker.joinable())
        return true;
    Event("debug-join");
    stop_debug = true;
    debug_worker.join();
    assert(!debug_alive);
    return true;
}
} // namespace mkw::debug
namespace mkw::switch_platform {
BootstrapInfo initialize() {
    Event("platform-start");
    debug_worker = std::thread([] {
        debug_alive = true;
        // Mimic the network worker's sole permitted action: publish a request.
        requested = true;
        while (!stop_debug)
            std::this_thread::yield();
        debug_alive = false;
    });
    watchdog_worker = std::thread([] {
        watchdog_alive = true;
        while (!stop_watchdog)
            std::this_thread::yield();
        watchdog_alive = false;
    });
    while (!debug_alive || !watchdog_alive)
        std::this_thread::yield();
    return {true, 1};
}
void present_bootstrap_screen(const BootstrapInfo&) {}
bool should_exit() {
    return false;
}
void shutdown() {
    assert(prepared && !debug_alive && !watchdog_alive && !memory_alive && !renderer_alive);
    Event("platform-stop");
}
} // namespace mkw::switch_platform
namespace mkw::horizon_runtime_services {
void sleep_for_ns(std::int64_t) noexcept {
    assert(renderer_alive && memory_alive && watchdog_alive);
    if (scenario == "retry") {
        assert(prepare_attempts == 1 && !prepared && debug_alive);
        requested = true; // A subsequent request retries on the same owner.
    } else {
        assert(scenario == "join-retry" && prepared);
        assert(debug_stop_attempts == 1 || watchdog_stop_attempts == 1);
    }
}
} // namespace mkw::horizon_runtime_services
namespace mkw::runtime_bootstrap {
Result start() {
    Event("runtime-start");
    memory_alive = true;
    guest_live = true;
    CpuContext cpu{};
    cpu.fpr[14].raw = 0x11223344;
    try {
        CpuContextScope scope(&cpu);
        PpcNonvolatileFprGuard fprs(&cpu);
        Guest(&cpu);
    } catch (const execution::ExecutionCancelled&) {
        assert(TryGetCpuContext() == nullptr && cpu.fpr[14].raw == 0x11223344);
        assert(guest_lock.try_lock());
        guest_lock.unlock();
        guest_live = false;
    }
    Result result;
    result.translated_execution_cancelled = true;
    result.guest_quiescent = true;
    return result;
}
bool write_report(const Result&) {
    Event("report");
    return true;
}
bool quiesce_guest() noexcept {
    assert(!guest_live && memory_alive);
    Event("quiesce");
    return true;
}
bool stop() noexcept {
    assert(prepared && !guest_live && !renderer_alive && !debug_alive && !watchdog_alive);
    Event("memory-stop");
    memory_alive = false;
    return true;
}
} // namespace mkw::runtime_bootstrap

int main(int argc, char** argv) {
    assert(argc == 2);
    scenario = argv[1];
    assert(scenario == "success" || scenario == "retry" || scenario == "join-retry");
    assert(std::atexit(ExitObserver) == 0);
    char launch[] = "sdmc:/switch/candidate.nro";
    char* candidate_args[] = {launch, nullptr};
    const int result = candidate_main(1, candidate_args);
    assert(result == 0);
    assert(prepare_attempts == (scenario == "retry" ? 2u : 1u));
    returned = true;
    return result;
}
