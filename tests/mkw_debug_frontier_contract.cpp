#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"
#include "mkw_debug_server.hpp"
#include "switch_guest_fiber.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
constexpr const char* directory = "sdmc:/switch/WiiCompiled-Switch/";
bool debug_enabled = false, disconnect_on_sleep = false;
unsigned polls = 0, sleeps = 0, durable_writes = 0;
std::uint64_t dispatch_count = 0;
std::uint32_t dispatch_pc = 0;
std::string stage, frontier, streamed;
std::string blocker() {
    std::ifstream in(std::string(directory) + "fast-track-dispatch-blocker.txt");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

std::uint64_t armGetSystemTick() {
    return 1000;
}
std::uint64_t armGetSystemTickFreq() {
    return 1000;
}
bool Memory::IsInitialized() noexcept {
    return false;
}
bool Memory::Contains(std::uint32_t, std::size_t) {
    return false;
}
std::uint8_t Memory::Read8(std::uint32_t) {
    std::abort();
}
std::uint16_t Memory::Read16(std::uint32_t) {
    std::abort();
}
std::uint32_t Memory::Read32(std::uint32_t) {
    std::abort();
}
std::uint8_t* Memory::GetPointer(std::uint32_t, std::size_t) {
    std::abort();
}
namespace GuestFlat {
std::uint8_t* Base() noexcept {
    return nullptr;
}
} // namespace GuestFlat
namespace mkw::switch_guest_fiber {
std::uint32_t current_thread() noexcept {
    return 0;
}
bool available() noexcept {
    return false;
}
bool has(std::uint32_t) noexcept {
    return false;
}
} // namespace mkw::switch_guest_fiber
namespace mkw::debug {
bool enabled() noexcept {
    return debug_enabled;
}
void note_stage(const char* value) noexcept {
    if (enabled())
        stage = value;
}
void note_dispatch(std::uint64_t count, std::uint32_t pc, const char* value) noexcept {
    if (!enabled())
        return;
    dispatch_count = count;
    dispatch_pc = pc;
    frontier = value;
}
void log(const char* data, std::size_t size) noexcept {
    if (enabled())
        streamed.append(data, size);
}
} // namespace mkw::debug
namespace mkw::horizon_runtime_services {
bool should_exit() {
    // A debugger must be able to fetch the blocker before the guest is held.
    assert(durable_writes > 0);
    assert(blocker().find("TEST_FRONTIER") != std::string::npos);
    assert(stage == "DEBUG_FRONTIER_PAUSED");
    assert(++polls <= 4);
    return polls == 4;
}
void sleep_for_ns(std::int64_t nanoseconds) noexcept {
    assert(nanoseconds == 50'000'000);
    assert(++sleeps <= 3);
    if (disconnect_on_sleep)
        debug_enabled = false;
}
} // namespace mkw::horizon_runtime_services
extern "C" int __wrap_fsync(int) {
    ++durable_writes;
    return 0;
}
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

int main(int argc, char** argv) {
    assert(argc == 2);
    debug_enabled = std::strcmp(argv[1], "disabled") != 0;
    disconnect_on_sleep = std::strcmp(argv[1], "disconnect") == 0;
    std::filesystem::create_directories(directory);
    CpuContext cpu{};
    cpu.pc = 0x8019B964;
    cpu.gpr[1] = 0x80399178;
    cpu.gpr[3] = 0x12345678;
    cpu.lr = 0x800060A4;
    CpuContext before = cpu;
    mkw_switch_set_fast_track_stage("TEST_RUNNING");
    mkw_switch_note_translated_dispatch(0x81234560, &cpu);
    mkw_switch_report_unsupported_translated_dispatch("TEST_FRONTIER", 0x81234560, &cpu);
    assert(std::memcmp(&before, &cpu, sizeof(cpu)) == 0);
    assert(blocker().find("TEST_FRONTIER") != std::string::npos);
    if (std::strcmp(argv[1], "disabled") == 0) {
        assert(polls == 0 && sleeps == 0);
        assert(dispatch_count == 0 && streamed.empty());
    } else {
        assert(polls == (disconnect_on_sleep ? 1u : 4u));
        assert(sleeps == (disconnect_on_sleep ? 1u : 3u));
        assert(dispatch_count == 1 && dispatch_pc == before.pc);
        assert(frontier == "TEST_FRONTIER@0x81234560");
        assert(streamed.find("TEST_FRONTIER") != std::string::npos);
    }
    std::puts("PASS: actual frontier reporter persists and streams evidence, preserves CPU, and holds only when debug is enabled");
}
