#include "gx_internal.h"

#include "gfx/common.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>

extern "C" void m3_probe_log(const char* message);

GxDisplayListState g_dlRecordState{};
std::atomic_bool g_auroraFrameActive{false};
std::atomic_bool g_auroraFrameHadWork{false};
bool g_alphaCompareValid = false;

namespace {

[[noreturn]] void unexpected_runtime_path(const char* message) {
    m3_probe_log(message);
    std::abort();
}

[[noreturn]] void throw_unmapped(uint32_t address, size_t length) {
    throw Memory::AccessViolation(address, length, "M3 HleFifoWrite probe has no guest RAM");
}

} // namespace

Memory::AccessViolation::AccessViolation(uint32_t address,
                                        size_t length,
                                        std::string_view reason)
    : std::runtime_error(std::string(reason)),
      address_(address),
      length_(length),
      reason_(reason) {}

uint8_t Memory::Read8(uint32_t addr) {
    throw_unmapped(addr, 1);
}

uint16_t Memory::Read16(uint32_t addr) {
    throw_unmapped(addr, 2);
}

uint32_t Memory::Read32(uint32_t addr) {
    throw_unmapped(addr, 4);
}

uint64_t Memory::Read64(uint32_t addr) {
    throw_unmapped(addr, 8);
}

float Memory::ReadFloat32(uint32_t addr) {
    throw_unmapped(addr, 4);
}

double Memory::ReadFloat64(uint32_t addr) {
    throw_unmapped(addr, 8);
}

void Memory::Write8(uint32_t addr, uint8_t) {
    throw_unmapped(addr, 1);
}

void Memory::Write16(uint32_t addr, uint16_t) {
    throw_unmapped(addr, 2);
}

void Memory::Write32(uint32_t addr, uint32_t) {
    throw_unmapped(addr, 4);
}

void Memory::Write64(uint32_t addr, uint64_t) {
    throw_unmapped(addr, 8);
}

void Memory::WriteFloat32(uint32_t addr, double) {
    throw_unmapped(addr, 4);
}

void Memory::WriteFloat64(uint32_t addr, double) {
    throw_unmapped(addr, 8);
}

uint8_t* Memory::GetPointer(uint32_t) {
    return nullptr;
}

uint8_t* Memory::GetPointer(uint32_t, size_t) {
    return nullptr;
}

bool Memory::Contains(uint32_t, size_t) {
    return false;
}

void* GuestToHostPtr(uint32_t, size_t) {
    return nullptr;
}

void BeginDisplayListRecording(uint32_t, uint32_t) {
    unexpected_runtime_path("FAIL unexpected BeginDisplayListRecording in HleFifoWrite probe\n");
}

void EndDisplayListRecording() {
    unexpected_runtime_path("FAIL unexpected EndDisplayListRecording in HleFifoWrite probe\n");
}

void WriteDisplayListData(uint32_t, uint32_t) {
    unexpected_runtime_path("FAIL unexpected display-list recording in HleFifoWrite probe\n");
}

void BeginNextAuroraFrameWithRetry(std::chrono::milliseconds) {
    if (!aurora::gfx::begin_frame()) {
        unexpected_runtime_path("FAIL HleFifoWrite requested Aurora frame but begin_frame failed\n");
    }
    g_auroraFrameActive.store(true, std::memory_order_release);
}

void EnsureAuroraFrameActive() {
    if (!g_auroraFrameActive.load(std::memory_order_acquire)) {
        BeginNextAuroraFrameWithRetry();
    }
}

extern "C" void GX__CallDisplayList_80172f64(uint32_t, uint32_t) {
    unexpected_runtime_path("FAIL unexpected GXCallDisplayList in HleFifoWrite probe\n");
}
