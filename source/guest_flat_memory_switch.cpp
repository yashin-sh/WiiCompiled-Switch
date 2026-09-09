#include "guest_flat_memory.h"
#include "horizon_guest_flat.hpp"

#include <switch.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace GuestFlat {
namespace {

mkw::horizon_guest_flat::Backing convert_backing(Backing backing) {
    switch (backing) {
    case Backing::Mem1:
        return mkw::horizon_guest_flat::Backing::Mem1;
    case Backing::Mem2:
        return mkw::horizon_guest_flat::Backing::Mem2;
    case Backing::Owned:
    default:
        return mkw::horizon_guest_flat::Backing::Owned;
    }
}

} // namespace

bool IsActive() {
    return mkw::horizon_guest_flat::is_active();
}

void Initialize(const std::vector<RegionRequest>& regions) {
    std::vector<mkw::horizon_guest_flat::RegionRequest> converted;
    converted.reserve(regions.size());
    for (const auto& region : regions) {
        converted.push_back({region.base, region.size, convert_backing(region.backing)});
    }

    mkw::horizon_guest_flat::InitReport report{};
    if (!mkw::horizon_guest_flat::initialize(converted.data(), converted.size(), &report)) {
        std::printf("FATAL: Horizon GuestFlat initialization failed: 0x%08x\n", report.result);
        std::printf("failed guest address: 0x%08x\n", report.failed_guest_address);
        consoleUpdate(nullptr);
        std::abort();
    }
}

std::uint8_t* HostPointer(std::uint32_t guestAddress) {
    return mkw::horizon_guest_flat::host_pointer(guestAddress);
}

std::uint8_t* Base() noexcept {
    return mkw::horizon_guest_flat::guest_base();
}

void ProtectDeferredRange(std::uint32_t, std::size_t) {
    // Deliberate no-op on Horizon. RequiresCheckedAccess() is always true, so
    // deferred reads remain on the checked Memory::* path instead of relying
    // on guest-view page faults.
}

void UnprotectDeferredRange(std::uint32_t, std::size_t) {
}

void RegisterExecutableRange(std::uint32_t, std::uint32_t) {
    // Deliberate no-op for the same reason: executable writes are handled by
    // the checked path on Switch rather than SharedMemory reprotection.
}

FaultCounters Counters() {
    return {};
}

void LogFaultSummary() noexcept {
}

bool HandleAccessViolation(void*, bool) noexcept {
    return false;
}

void Shutdown() noexcept {
    mkw::horizon_guest_flat::shutdown();
}

} // namespace GuestFlat
