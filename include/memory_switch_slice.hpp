#pragma once

// Nintendo-data-free Switch integration slice for WiiCompiled's Memory API.
// The constants and Config/RegionConfig model intentionally mirror upstream
// runtime/include/memory.h. This lets us validate the real Memory::Init mapping
// contract before importing the rest of the runtime dependency graph.

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class Memory {
  public:
    // Thrown (after logging) on accesses outside mapped regions. Mirrors the
    // upstream runtime's Memory::AccessViolation so shared decoder code
    // (HleFifoWrite display-list bursts) keeps its graceful-false handling
    // instead of just aborting.
    class AccessViolation : public std::runtime_error {
      public:
        AccessViolation(std::uint32_t address, std::size_t length, std::string_view reason);

        std::uint32_t address() const noexcept {
            return address_;
        }
        std::size_t length() const noexcept {
            return length_;
        }
        std::string_view reason() const noexcept {
            return reason_;
        }

      private:
        std::uint32_t address_ = 0;
        std::size_t length_ = 0;
        std::string reason_;
    };
    static constexpr std::size_t kMem1Size = 24u * 1024u * 1024u;
    static constexpr std::size_t kMem2Size = 128u * 1024u * 1024u;

    static constexpr std::uint32_t kMem1PhysicalBase = 0x00000000u;
    static constexpr std::uint32_t kMem1CachedBase = 0x80000000u;
    static constexpr std::uint32_t kMem1UncachedBase = 0xC0000000u;
    static constexpr std::uint32_t kMem2PhysicalBase = 0x10000000u;
    static constexpr std::uint32_t kMem2CachedBase = 0x90000000u;
    static constexpr std::uint32_t kMem2UncachedBase = 0xD0000000u;

    static constexpr std::uint32_t kMem2PhysicalEnd =
        kMem2PhysicalBase + static_cast<std::uint32_t>(kMem2Size);
    static constexpr std::uint32_t kMem2CachedEnd =
        kMem2CachedBase + static_cast<std::uint32_t>(kMem2Size);
    static constexpr std::uint32_t kMem2UncachedEnd =
        kMem2UncachedBase + static_cast<std::uint32_t>(kMem2Size);

    // Upstream WiiDefaults() adds a 1 MiB page plus 4 KiB successor for the
    // Broadway locked cache so its fast-path page classification can span the
    // page boundary safely.
    static constexpr std::uint32_t kLockedCacheBase = 0xE0000000u;
    static constexpr std::size_t kLockedCacheSize = 0x00101000u;

    struct RegionConfig {
        std::string name;
        std::uint32_t baseAddress = 0;
        std::size_t sizeBytes = 0;
    };

    struct Config {
        std::vector<RegionConfig> regions;
        static Config WiiDefaults();
    };

    // Mirrors the valid-path behavior of upstream Memory::Init: classify the
    // region backing, initialize GuestFlat, then bind each region to its
    // always-accessible host view. Invalid accesses abort loudly in this first
    // no-exceptions slice instead of silently returning bogus data.
    static void Init(const Config& config);
    static void Reset() noexcept;
    static bool IsInitialized() noexcept;

    static std::uint8_t Read8(std::uint32_t address);
    static std::uint16_t Read16(std::uint32_t address);
    static std::uint32_t Read32(std::uint32_t address);
    static std::uint64_t Read64(std::uint32_t address);

    static void Write8(std::uint32_t address, std::uint8_t value);
    static void Write16(std::uint32_t address, std::uint16_t value);
    static void Write32(std::uint32_t address, std::uint32_t value);
    static void Write64(std::uint32_t address, std::uint64_t value);

    static std::uint8_t* GetPointer(std::uint32_t address);
    static std::uint8_t* GetPointer(std::uint32_t address, std::size_t length);
    static bool Contains(std::uint32_t address, std::size_t length = 1);
    static std::vector<RegionConfig> DescribeRegions();
};
