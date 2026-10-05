#include "abi_bridge.h"
#include "switch_input_hle_traits.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

static_assert(KnownNativeCpuCall<0x801AF908u>::kAvailable);

namespace {
unsigned stages = 0;
unsigned opens = 0;
unsigned cases = 0;
bool failOpen = false;

void Invoke(std::uint32_t channel, std::uint32_t command, bool initialized) {
    using namespace mkw::switch_input_hle;
    g_pad_initialized = initialized;
    g_wpad_initialized = !initialized;
    g_wpad_dpd_sensitivity = 7;
    g_wpad_sync_device_callback = 0x12345678u;
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x39u + 23u * i);
    cpu.gpr[3] = channel;
    cpu.gpr[4] = command;
    CpuContext expected;
    std::memcpy(&expected, &cpu, sizeof(cpu));
    const auto before = stages;
    KnownNativeCpuCall<0x801AF908u>::Invoke(&cpu);
    assert(stages == before + 1);
    // Void ABI: all GPRs, FPRs, LR/CTR/CR/XER and padding must survive.
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    assert(g_pad_initialized == initialized && g_wpad_initialized == !initialized);
    assert(g_wpad_dpd_sensitivity == 7 && g_wpad_sync_device_callback == 0x12345678u);
    ++cases;
}

void CheckReport(const char* expected) {
    FILE* file = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-pad-control-motor.txt", "r");
    assert(file);
    char text[256]{};
    const auto length = std::fread(text, 1, sizeof(text) - 1, file);
    assert(!std::ferror(file) && std::feof(file));
    std::fclose(file);
    assert(length == std::strlen(expected) && std::strcmp(text, expected) == 0);
}
} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
    assert(std::strcmp(stage, "RMCP01_PAD_CONTROL_MOTOR") == 0);
    ++stages;
}
extern "C" FILE* __real_fopen(const char*, const char*);
extern "C" FILE* __wrap_fopen(const char* path, const char* mode) {
    assert(std::strcmp(path, "sdmc:/switch/WiiCompiled-Switch/fast-track-pad-control-motor.txt") == 0);
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
    KnownNativeCpuCall<0x801AF908u>::Invoke(nullptr);
    assert(stages == 0 && opens == 0);
    failOpen = true;
    Invoke(0, 2, false);
    assert(opens == 1);
    failOpen = false;
    Invoke(0, 2, true); // Failed SD opens must not suppress later evidence.
    assert(opens == 2);
    CheckReport("status=no-actuator-pass\nchannel=0\ncommand=2\nrumble_mask=0\nactuator_backend=absent\n");
    for (bool initialized : {false, true}) {
        for (auto channel : {0u, 1u, 2u, 3u, 4u, 255u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
            for (auto command : {0u, 1u, 2u, 3u, 255u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
                const auto before = opens;
                Invoke(channel, command, initialized);
                if (channel >= 4)
                    assert(opens == before);
            }
        }
    }
    for (unsigned channel = 0; channel < 4; ++channel)
        Invoke(channel, 2, true);
    CheckReport("status=no-actuator-pass\nchannel=3\ncommand=2\nrumble_mask=0\nactuator_backend=absent\n");
    const auto before = opens;
    for (unsigned frame = 0; frame < 1000; ++frame)
        for (unsigned channel = 0; channel < 4; ++channel)
            Invoke(channel, 2, frame % 2 != 0);
    assert(opens == before); // Four-channel polling must not reopen SD files.
    Invoke(0, 0, false);
    assert(opens == before + 1);
    Invoke(0, 2, false);
    assert(opens == before + 2);
    std::printf("PASS: PADControlMotor %u CPU/state/diagnostic cases; absent actuator, full void ABI, SD retries/cache\n",
                cases);
}
