#pragma once

// Switch/Horizon adaptation of WiiCompiled's GuestFlat public API.
//
// Upstream desktop targets use a compile-time fixed guest base. Horizon's
// address-space layout is randomized and the usable 4 GiB window is selected
// at runtime, so MKW_FLAT_GUEST_BASE resolves through Base() on Switch.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace GuestFlat {

inline constexpr std::uint64_t kGuestSpaceSize = 0x1'0000'0000ull;
inline constexpr std::size_t kGuestPageSize = 0x1000;

enum class Backing {
    Owned,
    Mem1,
    Mem2,
};

struct RegionRequest {
    std::uint32_t base = 0;
    std::uint64_t size = 0;
    Backing backing = Backing::Owned;
};

struct FaultCounters {
    std::uint32_t mmio = 0;
    std::uint32_t efb = 0;
    std::uint32_t xguard = 0;
    std::uint32_t unmapped = 0;
    std::uint32_t unmappedRegions = 0;
};

bool IsActive();

// Horizon SharedMemory cannot be reprotected with svcSetMemoryPermission.
// For the correctness-first Switch port, special and translated accesses stay
// on WiiCompiled's checked Memory::* path until a safe optimized policy is
// proven. This intentionally trades peak speed for an earlier stable boot.
inline constexpr bool RequiresCheckedAccess() noexcept { return true; }

void Initialize(const std::vector<RegionRequest>& regions);
std::uint8_t* HostPointer(std::uint32_t guestAddress);

// Runtime-selected base of the 4 GiB guest window. Null before Initialize().
std::uint8_t* Base() noexcept;

void ProtectDeferredRange(std::uint32_t address, std::size_t length);
void UnprotectDeferredRange(std::uint32_t address, std::size_t length);
void RegisterExecutableRange(std::uint32_t start, std::uint32_t end);

FaultCounters Counters();
void LogFaultSummary() noexcept;
bool HandleAccessViolation(void* faultAddress, bool isWrite) noexcept;

// Switch-only lifecycle extension used by smoke tests and clean application
// shutdown. Upstream desktop GuestFlat is process-lifetime today.
void Shutdown() noexcept;

} // namespace GuestFlat

#define MKW_FLAT_GUEST_BASE (GuestFlat::Base())
