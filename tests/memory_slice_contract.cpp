#include "guest_flat_memory.h"
#include "memory_switch_slice.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

namespace {
struct Allocation {
    std::uint32_t base;
    std::vector<std::uint8_t> bytes;
};
std::vector<Allocation> allocations;
bool active = false;
constexpr std::uint32_t base = 0x70000000u;
constexpr std::uint32_t lastRegion = 0xfffffff0u;
} // namespace

// Allocation-only host backing exercises the real Memory slice without a
// Switch VM, WiiDefaults allocations or game-derived bytes.
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    allocations.clear();
    for (const auto& request : requests) {
        assert(request.size <= 64u);
        allocations.push_back({request.base, std::vector<std::uint8_t>(request.size, 0xa5u)});
    }
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    for (auto& allocation : allocations) {
        if (address == allocation.base)
            return allocation.bytes.data();
    }
    return nullptr;
}
void Shutdown() noexcept {
    active = false;
    allocations.clear();
}
} // namespace GuestFlat

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}

int main() {
    assert(!Memory::GetPointer(base));
    Memory::Config config;
    config.regions = {{"synthetic", base, 16u}, {"last-guest-bytes", lastRegion, 16u}};
    Memory::Init(config);

    assert(Memory::Contains(base, 16u));
    assert(Memory::Contains(base + 15u, 1u));
    assert(Memory::Contains(lastRegion, 16u));
    assert(Memory::Contains(0xffffffffu, 1u));
    assert(!Memory::Contains(base, 0u));
    assert(!Memory::Contains(base - 1u, 2u));
    assert(!Memory::Contains(base, 17u));
    assert(!Memory::Contains(base + 15u, 2u));
    assert(!Memory::Contains(0xffffffffu, 2u));
    assert(!Memory::Contains(lastRegion, 17u));
    assert(!Memory::Contains(base, 0x100000000ull));

    // These lengths used to wrap start+length in uint64_t and falsely resolve
    // inside a tiny allocation. They must fail without forming a host range.
    constexpr auto maximum = std::numeric_limits<std::size_t>::max();
    for (const auto address : {base, base + 1u, lastRegion, 0xffffffffu}) {
        assert(!Memory::GetPointer(address, maximum));
        assert(!Memory::Contains(address, maximum - address + 1u));
    }

    // Unaligned operations use the guest's big-endian byte order and preserve
    // canaries, independently of a round-trip using the same implementation.
    Memory::Write32(base + 1u, 0x12345678u);
    assert(allocations[0].bytes[0] == 0xa5u);
    assert(allocations[0].bytes[1] == 0x12u);
    assert(allocations[0].bytes[2] == 0x34u);
    assert(allocations[0].bytes[3] == 0x56u);
    assert(allocations[0].bytes[4] == 0x78u);
    assert(allocations[0].bytes[5] == 0xa5u);
    allocations[1].bytes[14] = 0xabu;
    allocations[1].bytes[15] = 0xcdu;
    assert(Memory::Read16(0xfffffffeu) == 0xabcdu);

    bool caught = false;
    try {
        Memory::Write32(base + 14u, 0u);
    } catch (const Memory::AccessViolation& error) {
        caught = true;
        assert(error.address() == base + 14u);
        assert(error.length() == 4u);
    }
    assert(caught);
    assert(allocations[0].bytes[14] == 0xa5u);
    assert(allocations[0].bytes[15] == 0xa5u);

    Memory::Reset();
    assert(!Memory::IsInitialized());
    assert(!Memory::GetPointer(base));
    assert(!Memory::Contains(lastRegion, 16u));
}
