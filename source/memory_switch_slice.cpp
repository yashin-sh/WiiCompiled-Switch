#include "memory_switch_slice.hpp"

#include "guest_flat_memory.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

struct Region {
    Memory::RegionConfig config;
    std::uint8_t* storage = nullptr;
    std::size_t storage_size = 0;
};

std::vector<Region>& regions() {
    static std::vector<Region> value;
    return value;
}

bool& initialized() {
    static bool value = false;
    return value;
}

GuestFlat::Backing classify_backing(std::uint32_t base) {
    if ((base >= Memory::kMem1PhysicalBase &&
         base < Memory::kMem1PhysicalBase + Memory::kMem1Size) ||
        (base >= Memory::kMem1CachedBase &&
         base < Memory::kMem1CachedBase + Memory::kMem1Size) ||
        (base >= Memory::kMem1UncachedBase &&
         base < Memory::kMem1UncachedBase + Memory::kMem1Size)) {
        return GuestFlat::Backing::Mem1;
    }

    if ((base >= Memory::kMem2PhysicalBase && base < Memory::kMem2PhysicalEnd) ||
        (base >= Memory::kMem2CachedBase && base < Memory::kMem2CachedEnd) ||
        (base >= Memory::kMem2UncachedBase && base < Memory::kMem2UncachedEnd)) {
        return GuestFlat::Backing::Mem2;
    }

    return GuestFlat::Backing::Owned;
}

[[noreturn]] void invalid_access(std::uint32_t address, std::size_t length) {
    std::printf("FATAL: Memory access outside mapped regions: addr=0x%08x len=%zu\n",
                address, length);
    std::abort();
}

std::uint8_t* require_pointer(std::uint32_t address, std::size_t length) {
    if (auto* pointer = Memory::GetPointer(address, length)) {
        return pointer;
    }
    invalid_access(address, length);
}

} // namespace

Memory::Config Memory::Config::WiiDefaults() {
    Config config;
    config.regions.reserve(7);

    config.regions.push_back({"MEM1_PHYS", kMem1PhysicalBase, kMem1Size});
    config.regions.push_back({"MEM1_CACHED", kMem1CachedBase, kMem1Size});
    config.regions.push_back({"MEM1_UNCACHED", kMem1UncachedBase, kMem1Size});

    config.regions.push_back({"MEM2_PHYS", kMem2PhysicalBase, kMem2Size});
    config.regions.push_back({"MEM2_CACHED", kMem2CachedBase, kMem2Size});
    config.regions.push_back({"MEM2_UNCACHED", kMem2UncachedBase, kMem2Size});

    config.regions.push_back({"LOCKED_CACHE", kLockedCacheBase, kLockedCacheSize});
    return config;
}

void Memory::Init(const Config& config) {
    Reset();

    std::vector<GuestFlat::RegionRequest> requests;
    requests.reserve(config.regions.size());
    for (const auto& region : config.regions) {
        requests.push_back({region.baseAddress, region.sizeBytes,
                            classify_backing(region.baseAddress)});
    }

    GuestFlat::Initialize(requests);

    auto& active_regions = regions();
    active_regions.clear();
    active_regions.reserve(config.regions.size());

    for (const auto& config_region : config.regions) {
        std::uint8_t* host = nullptr;
        if (config_region.sizeBytes != 0) {
            host = GuestFlat::HostPointer(config_region.baseAddress);
            if (host == nullptr) {
                std::printf("FATAL: GuestFlat has no host view for %s at 0x%08x\n",
                            config_region.name.c_str(), config_region.baseAddress);
                std::abort();
            }
        }
        active_regions.push_back({config_region, host, config_region.sizeBytes});
    }

    initialized() = true;
}

void Memory::Reset() noexcept {
    initialized() = false;
    regions().clear();
    if (GuestFlat::IsActive()) {
        GuestFlat::Shutdown();
    }
}

bool Memory::IsInitialized() noexcept {
    return initialized();
}

std::uint8_t* Memory::GetPointer(std::uint32_t address) {
    return GetPointer(address, 1);
}

std::uint8_t* Memory::GetPointer(std::uint32_t address, std::size_t length) {
    if (!initialized() || length == 0) {
        return nullptr;
    }

    const std::uint64_t start = address;
    const std::uint64_t end = start + length;
    if (end > (std::uint64_t{1} << 32)) {
        return nullptr;
    }

    for (const auto& region : regions()) {
        const std::uint64_t base = region.config.baseAddress;
        const std::uint64_t limit = base + region.storage_size;
        if (start >= base && end <= limit) {
            return region.storage + static_cast<std::size_t>(start - base);
        }
    }
    return nullptr;
}

bool Memory::Contains(std::uint32_t address, std::size_t length) {
    return GetPointer(address, length) != nullptr;
}

std::vector<Memory::RegionConfig> Memory::DescribeRegions() {
    std::vector<RegionConfig> result;
    result.reserve(regions().size());
    for (const auto& region : regions()) {
        auto description = region.config;
        description.sizeBytes = region.storage_size;
        result.push_back(std::move(description));
    }
    return result;
}

std::uint8_t Memory::Read8(std::uint32_t address) {
    return *require_pointer(address, sizeof(std::uint8_t));
}

std::uint16_t Memory::Read16(std::uint32_t address) {
    std::uint16_t raw = 0;
    std::memcpy(&raw, require_pointer(address, sizeof(raw)), sizeof(raw));
    return __builtin_bswap16(raw);
}

std::uint32_t Memory::Read32(std::uint32_t address) {
    std::uint32_t raw = 0;
    std::memcpy(&raw, require_pointer(address, sizeof(raw)), sizeof(raw));
    return __builtin_bswap32(raw);
}

std::uint64_t Memory::Read64(std::uint32_t address) {
    std::uint64_t raw = 0;
    std::memcpy(&raw, require_pointer(address, sizeof(raw)), sizeof(raw));
    return __builtin_bswap64(raw);
}

void Memory::Write8(std::uint32_t address, std::uint8_t value) {
    *require_pointer(address, sizeof(value)) = value;
}

void Memory::Write16(std::uint32_t address, std::uint16_t value) {
    const std::uint16_t raw = __builtin_bswap16(value);
    std::memcpy(require_pointer(address, sizeof(raw)), &raw, sizeof(raw));
}

void Memory::Write32(std::uint32_t address, std::uint32_t value) {
    const std::uint32_t raw = __builtin_bswap32(value);
    std::memcpy(require_pointer(address, sizeof(raw)), &raw, sizeof(raw));
}

void Memory::Write64(std::uint32_t address, std::uint64_t value) {
    const std::uint64_t raw = __builtin_bswap64(value);
    std::memcpy(require_pointer(address, sizeof(raw)), &raw, sizeof(raw));
}
