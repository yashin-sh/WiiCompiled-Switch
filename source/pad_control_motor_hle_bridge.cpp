#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "hle/controller_status_contract.h"

#include <array>
#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
void Report(std::uint32_t channel, std::uint32_t command) {
    static std::array<bool, WpadContract::kChannelCount> reported{};
    static std::array<std::uint32_t, WpadContract::kChannelCount> previousCommand{};
    if (reported[channel] && previousCommand[channel] == command) {
        return;
    }
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-pad-control-motor.txt", "w")) {
        std::fprintf(out, "status=no-actuator-pass\nchannel=%u\ncommand=%u\nrumble_mask=0\nactuator_backend=absent\n",
                     channel, command);
        std::fclose(out);
        reported[channel] = true;
        previousCommand[channel] = command;
    }
}
} // namespace

extern "C" void mkw_switch_hle_pad_control_motor(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t channel = cpu->gpr[3];
    const std::uint32_t command = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_PAD_CONTROL_MOTOR");
    // Pinned PAD__ControlMotor_HLE forwards a void command to Aurora, whose
    // absent-controller branch returns before interpreting the command. This
    // Horizon backend has no actuator and PADRead reports rumble mask zero.
    // Preserve every CPU register, including the input arguments in r3/r4.
    // Invalid (including negative signed) channels likewise have no device.
    if (channel >= WpadContract::kChannelCount) {
        return;
    }
    Report(channel, command);
}

#endif
