#include "memory_init_probe.hpp"

#include "guest_flat_memory.h"
#include "memory_switch_slice.hpp"

#include <cstdio>

namespace mkw::memory_init_probe {
namespace {

constexpr const char* kReportPath = "sdmc:/switch/WiiCompiled-Switch/vm-probe.txt";

const char* ok(bool value) {
    return value ? "OK" : "FAILED";
}

} // namespace

Result run() {
    Result result{};

    auto config = Memory::Config::WiiDefaults();
    Memory::Init(config);

    result.initialized = Memory::IsInitialized() && GuestFlat::IsActive();
    result.runtime_guest_base = GuestFlat::Base() != nullptr;
    result.checked_access_policy = GuestFlat::RequiresCheckedAccess();

    const auto described = Memory::DescribeRegions();
    result.described_regions = described.size();
    result.region_count = config.regions.size() == 7 && described.size() == 7;

    result.contains_policy =
        Memory::Contains(Memory::kMem1CachedBase + 0x100u, 8) &&
        Memory::Contains(Memory::kMem2CachedBase + 0x200u, 8) &&
        Memory::Contains(Memory::kLockedCacheBase + 0x20u, 2) &&
        !Memory::Contains(0x70000000u, 4);

    constexpr std::uint32_t mem1_offset = 0x100u;
    Memory::Write32(Memory::kMem1CachedBase + mem1_offset, 0x12345678u);
    auto* mem1_phys = Memory::GetPointer(Memory::kMem1PhysicalBase + mem1_offset, 4);
    auto* mem1_cached = Memory::GetPointer(Memory::kMem1CachedBase + mem1_offset, 4);
    auto* mem1_uncached = Memory::GetPointer(Memory::kMem1UncachedBase + mem1_offset, 4);
    result.mem1_aliases =
        mem1_phys != nullptr && mem1_cached != nullptr && mem1_uncached != nullptr &&
        mem1_phys == mem1_cached && mem1_phys == mem1_uncached &&
        Memory::Read32(Memory::kMem1PhysicalBase + mem1_offset) == 0x12345678u &&
        Memory::Read32(Memory::kMem1UncachedBase + mem1_offset) == 0x12345678u;
    result.mem1_big_endian =
        mem1_phys != nullptr &&
        mem1_phys[0] == 0x12 && mem1_phys[1] == 0x34 &&
        mem1_phys[2] == 0x56 && mem1_phys[3] == 0x78;

    constexpr std::uint32_t mem2_offset = 0x200u;
    Memory::Write32(Memory::kMem2CachedBase + mem2_offset, 0x89ABCDEFu);
    auto* mem2_phys = Memory::GetPointer(Memory::kMem2PhysicalBase + mem2_offset, 4);
    auto* mem2_cached = Memory::GetPointer(Memory::kMem2CachedBase + mem2_offset, 4);
    auto* mem2_uncached = Memory::GetPointer(Memory::kMem2UncachedBase + mem2_offset, 4);
    result.mem2_aliases =
        mem2_phys != nullptr && mem2_cached != nullptr && mem2_uncached != nullptr &&
        mem2_phys == mem2_cached && mem2_phys == mem2_uncached &&
        Memory::Read32(Memory::kMem2PhysicalBase + mem2_offset) == 0x89ABCDEFu &&
        Memory::Read32(Memory::kMem2UncachedBase + mem2_offset) == 0x89ABCDEFu;
    result.mem2_big_endian =
        mem2_phys != nullptr &&
        mem2_phys[0] == 0x89 && mem2_phys[1] == 0xAB &&
        mem2_phys[2] == 0xCD && mem2_phys[3] == 0xEF;

    constexpr std::uint32_t locked_offset = 0x20u;
    Memory::Write16(Memory::kLockedCacheBase + locked_offset, 0xBEEFu);
    auto* locked = Memory::GetPointer(Memory::kLockedCacheBase + locked_offset, 2);
    result.locked_cache =
        locked != nullptr &&
        locked[0] == 0xBE && locked[1] == 0xEF &&
        Memory::Read16(Memory::kLockedCacheBase + locked_offset) == 0xBEEFu;

    Memory::Reset();
    result.reset = !Memory::IsInitialized() && !GuestFlat::IsActive() && GuestFlat::Base() == nullptr;

    return result;
}

void print(const Result& result) {
    std::printf("\nWiiCompiled Memory::Init Horizon smoke test\n");
    std::printf("==========================================\n");
    std::printf("initialized          : %s\n", ok(result.initialized));
    std::printf("described regions    : %zu / 7\n", result.described_regions);
    std::printf("region count         : %s\n", ok(result.region_count));
    std::printf("runtime guest base   : %s\n", ok(result.runtime_guest_base));
    std::printf("checked-access policy: %s\n", ok(result.checked_access_policy));
    std::printf("Contains policy      : %s\n", ok(result.contains_policy));
    std::printf("MEM1 aliases         : %s\n", ok(result.mem1_aliases));
    std::printf("MEM1 big-endian      : %s\n", ok(result.mem1_big_endian));
    std::printf("MEM2 aliases         : %s\n", ok(result.mem2_aliases));
    std::printf("MEM2 big-endian      : %s\n", ok(result.mem2_big_endian));
    std::printf("locked cache         : %s\n", ok(result.locked_cache));
    std::printf("reset                : %s\n", ok(result.reset));
    std::printf("Memory::Init smoke   : %s\n", result.passed() ? "PASS" : "FAIL");
}

bool append_report(const Result& result) {
    std::FILE* file = std::fopen(kReportPath, "a");
    if (!file) {
        return false;
    }

    std::fprintf(file, "\nWiiCompiled Memory::Init Horizon smoke test\n");
    std::fprintf(file, "==========================================\n");
    std::fprintf(file, "initialized          : %s\n", ok(result.initialized));
    std::fprintf(file, "described regions    : %zu / 7\n", result.described_regions);
    std::fprintf(file, "region count         : %s\n", ok(result.region_count));
    std::fprintf(file, "runtime guest base   : %s\n", ok(result.runtime_guest_base));
    std::fprintf(file, "checked-access policy: %s\n", ok(result.checked_access_policy));
    std::fprintf(file, "Contains policy      : %s\n", ok(result.contains_policy));
    std::fprintf(file, "MEM1 aliases         : %s\n", ok(result.mem1_aliases));
    std::fprintf(file, "MEM1 big-endian      : %s\n", ok(result.mem1_big_endian));
    std::fprintf(file, "MEM2 aliases         : %s\n", ok(result.mem2_aliases));
    std::fprintf(file, "MEM2 big-endian      : %s\n", ok(result.mem2_big_endian));
    std::fprintf(file, "locked cache         : %s\n", ok(result.locked_cache));
    std::fprintf(file, "reset                : %s\n", ok(result.reset));
    std::fprintf(file, "Memory::Init smoke   : %s\n", result.passed() ? "PASS" : "FAIL");

    std::fclose(file);
    return true;
}

} // namespace mkw::memory_init_probe
