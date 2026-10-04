#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "hle/controller_status_contract.h"
#include "memory.h"

#include <cstdio>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
void Report(const char* status, std::uint32_t channel, std::uint32_t address, std::int32_t result) {
    static const char* previousStatus = nullptr;
    static std::uint32_t previousChannel = 0;
    static std::uint32_t previousAddress = 0;
    if (previousStatus == status && previousChannel == channel && previousAddress == address) {
        return;
    }
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-wpad-probe.txt", "w")) {
        std::fprintf(out, "status=%s\nchannel=%u\ntype_buffer=0x%08x\nresult=%d\nextension_type=0\nremote_backend=absent\n",
                     status, channel, address, result);
        std::fclose(out);
        previousStatus = status;
        previousChannel = channel;
        previousAddress = address;
    }
}
} // namespace

extern "C" void mkw_switch_hle_wpad_probe(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t channel = cpu->gpr[3];
    const std::uint32_t typeAddress = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_WPAD_PROBE");
    if (channel >= WpadContract::kChannelCount) {
        // The pinned HLE checks the channel before polling or accessing output.
        cpu->gpr[3] = static_cast<std::uint32_t>(WpadContract::kErrorBadChannel);
        return;
    }
    if (typeAddress) {
        if (!Memory::Contains(typeAddress, sizeof(std::uint32_t))) {
            Report("invalid-type-range", channel, typeAddress, WpadContract::kErrorNoController);
            mkw_switch_report_unsupported_translated_dispatch("WPAD_PROBE_TYPE_RANGE", 0x801C0990u, cpu);
            std::abort();
        }
        Memory::Write32(typeAddress, WpadContract::kExtensionCore);
    }
    // The existing Joy-Con/default controller is on the PAD path. It is not a
    // Bluetooth Wiimote. No desktop Poll(), callback or library-state mutation.
    cpu->gpr[3] = static_cast<std::uint32_t>(WpadContract::kErrorNoController);
    Report("no-remote-pass", channel, typeAddress, WpadContract::kErrorNoController);
}

#endif
