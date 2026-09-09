#pragma once

#include <cstddef>
#include <cstdint>

namespace mkw::horizon_guest_flat {

inline constexpr std::uint64_t kGuestSpaceSize = 0x1'0000'0000ull;
inline constexpr std::size_t kGuestPageSize = 0x1000;

enum class Backing : std::uint8_t {
    Owned,
    Mem1,
    Mem2,
};

struct RegionRequest {
    std::uint32_t base = 0;
    std::uint64_t size = 0;
    Backing backing = Backing::Owned;
};

struct InitReport {
    bool guest_window_found = false;
    bool guest_window_reserved = false;
    std::uintptr_t guest_base = 0;
    std::size_t section_count = 0;
    std::size_t region_count = 0;
    std::size_t guest_mapping_count = 0;
    std::uint32_t result = 0;
    std::uint32_t failed_guest_address = 0;
};

// Reserves one contiguous 4 GiB guest VA window for the translated-address
// contract, while storing MEM1/MEM2/Owned backing inside the loader-provided
// heap. Horizon full-size Wii backings intentionally use checked HostPointer
// access rather than large SharedMemory objects: hbloader's mandatory heap
// override already owns the process memory budget and svcCreateSharedMemory
// for a 24 MiB MEM1 object can hit Kernel LimitReached.
bool initialize(const RegionRequest* regions, std::size_t count, InitReport* report = nullptr);

bool is_active();
std::uint8_t* guest_base();

// Returns the always-accessible host view for a requested Wii guest address,
// folding physical/cached/uncached aliases onto the same backing store.
std::uint8_t* host_pointer(std::uint32_t guest_address);

// Switch currently runs the translated memory path in correctness-first checked
// mode. The reserved guest base is an address-space token, not a directly
// dereferenceable 4 GiB mirror.
constexpr bool requires_checked_access_for_special_ranges() noexcept { return true; }

void shutdown();

struct SmokeResult {
    InitReport init{};
    bool initialized = false;
    bool mem1_host_to_cached_guest = false;
    bool mem1_uncached_guest_to_host = false;
    bool mem1_host_alias_coherent = false;
    bool mem2_host_to_cached_guest = false;
    bool mem2_uncached_guest_to_host = false;
    bool mem2_host_alias_coherent = false;
    bool owned_host_to_guest = false;
    bool teardown_ok = false;

    bool passed() const {
        return initialized &&
               mem1_host_to_cached_guest &&
               mem1_uncached_guest_to_host &&
               mem1_host_alias_coherent &&
               mem2_host_to_cached_guest &&
               mem2_uncached_guest_to_host &&
               mem2_host_alias_coherent &&
               owned_host_to_guest &&
               teardown_ok;
    }
};

// Hardware smoke test using synthetic data only. It validates checked alias
// resolution for Wii MEM1/MEM2 plus an Owned region without Nintendo data.
SmokeResult run_smoke_test();
void print_smoke_result(const SmokeResult& result);
bool append_smoke_report(const SmokeResult& result);

} // namespace mkw::horizon_guest_flat
