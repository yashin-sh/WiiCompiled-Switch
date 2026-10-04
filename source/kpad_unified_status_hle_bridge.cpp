#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "hle/controller_status_contract.h"
#include "memory.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
// Pinned runtime/src/hle/input/kpad.cpp: KPADUnifiedWpadStatus union and fmt.
constexpr std::size_t kStatusSize = 0x38;
constexpr std::uint32_t kMaxEntries = 16;
constexpr auto kAbsentStatus = [] {
    std::array<std::uint8_t, kStatusSize> value{};
    value[0x29] = 0xff; // WPAD_ERR_NO_CONTROLLER; core device type stays 0.
    value[0x36] = 2;    // WPAD_FMT_CORE_ACC_DPD; padding/extension stay zero.
    return value;
}();

void Report(const char* status, std::uint32_t channel, std::uint32_t address, std::uint32_t requested,
            std::uint32_t written, std::size_t checkedBytes) {
    static std::array<const char*, WpadContract::kChannelCount> previousStatus{};
    static std::array<std::uint32_t, WpadContract::kChannelCount> previousAddress{};
    static std::array<std::uint32_t, WpadContract::kChannelCount> previousCount{};
    if (previousStatus[channel] == status && previousAddress[channel] == address && previousCount[channel] == requested) {
        return;
    }
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-kpad-unified-status.txt", "w")) {
        std::fprintf(out,
                     "status=%s\nchannel=%u\nstatus_buffer=0x%08x\nentries_requested=%u\nentries_written=%u\n"
                     "checked_bytes=%zu\nreturn_count=0\nremote_backend=absent\n",
                     status, channel, address, requested, written, checkedBytes);
        std::fclose(out);
        previousStatus[channel] = status;
        previousAddress[channel] = address;
        previousCount[channel] = requested;
    }
}
} // namespace

extern "C" void mkw_switch_hle_kpad_unified_status(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t channel = cpu->gpr[3];
    const std::uint32_t address = cpu->gpr[4];
    const std::uint32_t requested = cpu->gpr[5];
    mkw_switch_set_fast_track_stage("RMCP01_KPAD_UNIFIED_STATUS");
    if (channel >= WpadContract::kChannelCount || !address || !requested) {
        cpu->gpr[3] = 0;
        return;
    }
    const std::uint32_t entries = std::min(requested, kMaxEntries);
    const std::size_t bytes = entries * kStatusSize;
    auto* output = Memory::GetPointer(address, bytes);
    if (!output) {
        // Validate all entries before writing; never leave a partial sample.
        Report("invalid-status-range", channel, address, requested, 0, bytes);
        mkw_switch_report_unsupported_translated_dispatch("KPAD_UNIFIED_STATUS_RANGE", 0x8019812Cu, cpu);
        std::abort();
    }
    for (std::uint32_t i = 0; i < entries; ++i) {
        std::memcpy(output + i * kStatusSize, kAbsentStatus.data(), kStatusSize);
    }
    // A Joy-Con is on PADRead, not the unavailable Wii Bluetooth backend.
    cpu->gpr[3] = 0;
    Report("no-remote-pass", channel, address, requested, entries, bytes);
}

#endif
