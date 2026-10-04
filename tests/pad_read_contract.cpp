#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory_switch_slice.hpp"
#include "switch_input_hle_traits.hpp"
#include <dolphin/pad.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

// Independent packed expectations below are anchored to the pinned SDK.
static_assert(PAD_TRIGGER_L == 0x40 && PAD_TRIGGER_R == 0x20 && PAD_TRIGGER_Z == 0x10);
static_assert(PAD_BUTTON_A == 0x100 && PAD_BUTTON_B == 0x200);
static_assert(PAD_BUTTON_X == 0x400 && PAD_BUTTON_Y == 0x800 && PAD_BUTTON_START == 0x1000);
static_assert(PAD_BUTTON_LEFT == 1 && PAD_BUTTON_RIGHT == 2 && PAD_BUTTON_DOWN == 4 && PAD_BUTTON_UP == 8);
static_assert(PAD_CHANMAX == 4 && PAD_ERR_NO_CONTROLLER == -1);
static_assert(KnownNativeCpuCall<0x801AF44Cu>::kAvailable);
namespace {
constexpr std::uint32_t base = 0x70000000u;
std::array<std::uint8_t, 64> backing;
bool active = false;
unsigned polls = 0;
unsigned cases = 0;
unsigned stages = 0;
mkw::horizon_runtime_services::InputState input{};
using Packet = std::array<std::uint8_t, 48>;

Packet Absent() {
    Packet bytes{};
    for (unsigned port = 0; port < 4; ++port)
        bytes[12 * port + 10] = 0xff;
    return bytes;
}
Packet Connected() {
    auto bytes = Absent();
    bytes[10] = 0;
    return bytes;
}

void Invoke(std::uint32_t address, bool expectPoll) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x39u + 23u * i);
    cpu.gpr[3] = address;
    CpuContext expected;
    std::memcpy(&expected, &cpu, sizeof(cpu));
    expected.gpr[3] = 0;
    const auto previousPolls = polls;
    const auto previousStages = stages;
    KnownNativeCpuCall<0x801AF44Cu>::Invoke(&cpu);
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(polls == previousPolls + (expectPoll ? 1u : 0u));
    assert(stages == previousStages + 1u);
    ++cases;
}

void Check(const Packet& expected) {
    backing.fill(0xa5);
    Invoke(base + 1, true); // Deliberately unaligned, with both-side canaries.
    assert(std::memcmp(backing.data() + 1, expected.data(), 48) == 0);
    assert(backing[0] == 0xa5);
    for (unsigned i = 49; i < backing.size(); ++i)
        assert(backing[i] == 0xa5);
}
} // namespace

// Allocation and device seams only. Production CPU bridge, Memory slice and
// the unchanged pinned PADStatus encoder execute with no game-derived input.
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == backing.size());
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == base ? backing.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
}
} // namespace GuestFlat
namespace mkw::horizon_runtime_services {
InputState poll_input() {
    ++polls;
    return input;
}
} // namespace mkw::horizon_runtime_services
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    if (std::strcmp(stage, "RMCP01_PAD_READ") == 0)
        ++stages;
}

int main() {
    const auto previousStages = stages;
    KnownNativeCpuCall<0x801AF44Cu>::Invoke(nullptr);
    assert(stages == previousStages && polls == 0);
    Invoke(0, false);
    Invoke(base, false); // Uninitialized Memory: cannot poll or partially write.
    Memory::Config config;
    config.regions = {{"PAD status test", base, backing.size()}};
    Memory::Init(config);
    for (const auto address : {base - 1, base + 17, base + 63, 0xfffffff0u}) {
        backing.fill(0xa5);
        Invoke(address, false);
        for (const auto byte : backing)
            assert(byte == 0xa5);
    }
    Check(Absent()); // Uninitialized/disconnected host input is absent.
    input.connected = true;
    Check(Connected()); // Neutral controller is distinct from absent.

    // Independent libnx bit numbers and GC SDK button masks. Verify every
    // mapping alone and ensure StickL/StickR, Minus and synthetic axis bits
    // cannot turn into extra GC buttons.
    constexpr std::array<std::pair<unsigned, unsigned>, 13> buttonCases{{
        {0, 0x100},
        {1, 0x200},
        {2, 0x400},
        {3, 0x800},
        {6, 0x40},
        {7, 0x20},
        {8, 0x10},
        {9, 0x10},
        {10, 0x1000},
        {12, 1},
        {13, 8},
        {14, 2},
        {15, 4},
    }};
    for (const auto& [bit, mask] : buttonCases) {
        input.buttons_held = 1ull << bit;
        auto expected = Connected();
        expected[0] = mask >> 8;
        expected[1] = mask & 0xff;
        expected[6] = bit == 6 ? 255 : 0;
        expected[7] = bit == 7 ? 255 : 0;
        Check(expected);
    }
    input.buttons_held = (1ull << 4) | (1ull << 5) | (1ull << 11) | (1ull << 16) | (1ull << 63);
    input.buttons_down = ~0ull;
    Check(Connected()); // Held inputs, not edge events, determine PADRead.
    input.buttons_held = 0;

    // Full signed HID domain, independently defined scale/endpoint bound.
    // Exercise all four axes together and both signs without SDL Y inversion.
    for (int value = -32768; value <= 32767; ++value) {
        input.left_x = value;
        input.left_y = -value;
        input.right_x = value;
        input.right_y = -value;
        auto expected = Connected();
        const auto reference = [](int v) {
            const int magnitude = (v < 0 ? -v : v) / 256;
            return static_cast<std::uint8_t>((v < 0 ? -1 : 1) * (magnitude > 127 ? 127 : magnitude));
        };
        expected[2] = expected[4] = reference(value);
        expected[3] = expected[5] = reference(-value);
        Check(expected);
    }
    input.left_x = input.right_y = std::numeric_limits<int>::min();
    input.left_y = input.right_x = std::numeric_limits<int>::max();
    auto endpoints = Connected();
    endpoints[2] = endpoints[5] = 0x81;
    endpoints[3] = endpoints[4] = 0x7f;
    Check(endpoints); // Out-of-contract host values remain bounded.
    input.buttons_held = ~0ull;
    auto all = endpoints;
    all[0] = 0x1f;
    all[1] = 0x7f;
    all[6] = all[7] = 255;
    Check(all);
    input.connected = false;
    Check(Absent()); // Stale held buttons/axes must not leak on disconnect.
    input = {};
    input.connected = true;
    Check(Connected()); // Reconnect resets all fields, including padding.
    Memory::Reset();
    backing.fill(0xa5);
    Invoke(base + 1, false);
    for (const auto byte : backing)
        assert(byte == 0xa5);
    std::printf("PASS: PADRead %u CPU/memory/input cases; all four packed slots, axes, buttons and disconnects\n", cases);
}
