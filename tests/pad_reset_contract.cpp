#include "abi_bridge.h"
#include "switch_input_hle_traits.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

static_assert(KnownNativeCpuCall<0x801AF0DCu>::kAvailable);

namespace {
unsigned stages = 0;
unsigned opens = 0;
unsigned cases = 0;
bool failOpen = false;

void Invoke(std::uint32_t mask, bool initialized) {
    using namespace mkw::switch_input_hle;
    g_pad_initialized = initialized;
    g_wpad_initialized = !initialized;
    g_wpad_dpd_sensitivity = 7;
    g_wpad_sync_device_callback = 0xabcdef01u;
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(mask + 37u * i);
    cpu.gpr[3] = mask;
    CpuContext expected;
    std::memcpy(&expected, &cpu, sizeof(cpu));
    expected.gpr[3] = 1;
    const auto before = stages;
    KnownNativeCpuCall<0x801AF0DCu>::Invoke(&cpu);
    assert(stages == before + 1);
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(g_pad_initialized == initialized && g_wpad_initialized == !initialized);
    assert(g_wpad_dpd_sensitivity == 7 && g_wpad_sync_device_callback == 0xabcdef01u);
    ++cases;
}

void CheckObservedReport() {
    FILE* file = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-pad-reset.txt", "r");
    assert(file);
    char text[256]{};
    const auto length = std::fread(text, 1, sizeof(text) - 1, file);
    assert(!std::ferror(file) && std::feof(file));
    std::fclose(file);
    constexpr char expected[] = "status=pinned-reset-pass\nmask=0x70000000\nreturn_value=1\ncontroller_reset=none\n";
    assert(length == sizeof(expected) - 1 && std::strcmp(text, expected) == 0);
}
} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    assert(std::strcmp(stage, "RMCP01_PAD_RESET") == 0);
    ++stages;
}
extern "C" FILE* __real_fopen(const char*, const char*);
extern "C" FILE* __wrap_fopen(const char* path, const char* mode) {
    assert(std::strcmp(path, "sdmc:/switch/WiiCompiled-Switch/fast-track-pad-reset.txt") == 0);
    if (std::strcmp(mode, "w") == 0) {
        ++opens;
        if (failOpen)
            return nullptr;
    } else {
        assert(std::strcmp(mode, "r") == 0);
    }
    return __real_fopen(path, mode);
}

int main() {
    KnownNativeCpuCall<0x801AF0DCu>::Invoke(nullptr);
    assert(stages == 0 && opens == 0);
    failOpen = true;
    Invoke(0x70000000u, false);
    assert(opens == 1);
    failOpen = false;
    Invoke(0x70000000u, true);
    assert(opens == 2);
    CheckObservedReport();
    for (bool initialized : {false, true}) {
        for (auto mask : {0u, 1u, 3u, 7u, 15u, 255u, 0x0fffffffu, 0x70000000u, 0x80000000u, 0xf0000000u,
                          0xfffffff0u, 0x7fffffffu, 0xffffffffu})
            Invoke(mask, initialized);
        for (unsigned bit = 0; bit < 32; ++bit)
            Invoke(1u << bit, initialized);
        for (unsigned ports = 0; ports < 16; ++ports)
            Invoke(ports << 28, initialized);
        std::uint32_t value = 0x31415926u;
        for (unsigned i = 0; i < 2048; ++i) {
            value ^= value << 13;
            value ^= value >> 17;
            value ^= value << 5;
            Invoke(value, initialized);
        }
    }
    Invoke(0x70000000u, true);
    CheckObservedReport();
    const auto before = opens;
    for (unsigned frame = 0; frame < 1000; ++frame)
        Invoke(0x70000000u, frame % 2 != 0);
    assert(opens == before);
    Invoke(0u, false);
    assert(opens == before + 1);
    Invoke(0x70000000u, false);
    assert(opens == before + 2);
    CheckObservedReport();
    std::printf("PASS: PADReset %u CPU/state/diagnostic cases; full-width masks, pinned success, SD retries/cache\n",
                cases);
}
