#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"
#include <switch.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <unistd.h>

namespace {
std::uint64_t tick = 0, frequency = 1000;
std::map<std::string, unsigned> syncs;
constexpr const char* dir = "sdmc:/switch/WiiCompiled-Switch/";
std::string Report(const char* name) {
    std::ifstream in(std::string(dir) + name);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
unsigned Syncs(const char* name) {
    return syncs[std::string(dir) + name];
}
} // namespace
std::uint64_t armGetSystemTick() {
    return tick;
}
std::uint64_t armGetSystemTickFreq() {
    return frequency;
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
extern "C" int __wrap_fsync(int fd) {
    char path[4096];
    const auto link = "/proc/self/fd/" + std::to_string(fd);
    const auto length = readlink(link.c_str(), path, sizeof(path) - 1);
    assert(length > 0);
    path[length] = 0;
    const char* relative = std::strstr(path, dir);
    assert(relative);
    ++syncs[relative];
    return 0;
}
extern "C" void __libnx_exception_handler(ThreadExceptionDump*);
int main(int argc, char**) {
    if (argc > 1)
        frequency = 0;
    std::filesystem::create_directories(dir);
    CpuContext cpu{};
    cpu.pc = 0x800060a4;
    mkw_switch_note_translated_dispatch(0x8000b6b0, &cpu);
    // A million repeated scheduler/GX phase hits at tick zero must retain
    // startup evidence without turning zero into an uninitialized sentinel.
    for (unsigned i = 0; i < 1000000; ++i)
        mkw_switch_note_translated_dispatch(i % 2 ? 0x801aaaa4 : 0x80173214, &cpu);
    assert(Syncs("fast-track-post-main-last-dispatch.txt") == 16);
    assert(Syncs("fast-track-heartbeat.txt") == 2);
    const auto before = Report("fast-track-post-main-last-dispatch.txt");
    tick = 999;
    mkw_switch_note_translated_dispatch(0x801aaaa4, &cpu);
    assert(Report("fast-track-post-main-last-dispatch.txt") == before);
    tick = 1000;
    mkw_switch_note_translated_dispatch(0x80008ef0, &cpu);
    assert(Syncs("fast-track-post-main-last-dispatch.txt") == (frequency ? 17 : 16));
    if (frequency) {
        assert(Report("fast-track-post-main-last-dispatch.txt").find("target           : 0x80008ef0") != std::string::npos);
        assert(Report("fast-track-post-main-last-dispatch.txt").find("dispatch count        : 1000003") != std::string::npos);
    }
    tick = 900; // Clock rollback must not cause unsigned-underflow writes.
    for (unsigned i = 0; i < 10000; ++i)
        mkw_switch_note_translated_dispatch(0x801aaaa4, &cpu);
    assert(Syncs("fast-track-post-main-last-dispatch.txt") == (frequency ? 17 : 16));
    tick = 2000;
    mkw_switch_note_translated_dispatch(0x80173214, &cpu);
    assert(Syncs("fast-track-post-main-last-dispatch.txt") == (frequency ? 18 : 16));
    // Terminal evidence must bypass the periodic snapshot budget immediately.
    mkw_switch_report_unsupported_translated_dispatch("TEST_BLOCKER", 0x81234560, &cpu);
    assert(Syncs("fast-track-dispatch-blocker.txt") == 1);
    assert(Report("fast-track-dispatch-blocker.txt").find("TEST_BLOCKER") != std::string::npos);
    ThreadExceptionDump exception{};
    exception.error_desc = 0x1234;
    __libnx_exception_handler(&exception);
    assert(Syncs("fast-track-exception.txt") == 1);
    assert(Report("fast-track-exception.txt").find("0x00001234") != std::string::npos);
    std::puts("PASS: real diagnostic writer bounds SD syncs, retains counters and writes terminal evidence immediately");
}
